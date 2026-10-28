// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>
#include <limits>
#include <thread>
#include "FxController.h"
#include "AudioRouteLease.h"
#include "DeviceValidation.h"

namespace {
void fillTone(juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample(ch, i, 0.15f * std::sin((float)i * 0.061f));
}
bool legal(const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (!std::isfinite(buffer.getSample(ch, i)) || std::abs(buffer.getSample(ch, i)) > 1.0f) return false;
    return true;
}
struct FakeRouting : AudioRoutingBackend
{
    juce::String current { "speakers" };
    juce::StringArray connected { "speakers", "headphones", "capture" };
    bool failWrites = false;
    juce::String defaultOutput() const override { return current; }
    bool available(const juce::String& uid) const override { return connected.contains(uid); }
    bool setDefaultOutput(const juce::String& uid) override {
        if (failWrites || !available(uid)) return false;
        current = uid; return true;
    }
    juce::String fallbackOutput() const override { return available("speakers") ? "speakers" : ""; }
};
}

struct ReliabilityTests : juce::UnitTest
{
    ReliabilityTests() : juce::UnitTest("Mac reliability") {}
    void runTest() override
    {
        beginTest("strict capture selection and feedback prevention");
        expect(DeviceValidation::findBlackHoleDevice({"BlackHole 16ch", "BlackHole 2ch"}) == "BlackHole 2ch");
        expect(DeviceValidation::findBlackHoleDevice({"Microphone", "BlackHole 16ch"}).isEmpty());
        expect(DeviceValidation::validateRoute("BlackHole 2ch", "BlackHole 2ch", {"BlackHole 2ch"}).state == AudioEngineState::InvalidOutput);
        expect(DeviceValidation::validateRoute("BlackHole 2ch", "gone", {"speakers"}).state == AudioEngineState::DeviceUnavailable);
        expect(!DeviceValidation::sampleRatesMatch(-1, -1));

        beginTest("invalid format and empty buffers fail safely");
        FxController controller;
        expect(!controller.prepare(0, 64));
        expect(!controller.prepare(48000, 0));
        expect(controller.prepare(48000, 64));
        juce::AudioBuffer<float> empty;
        controller.processBlock(empty);
        controller.setOutputGainDb(-3.0f);
        controller.setOutputGainDb(std::numeric_limits<float>::quiet_NaN());
        expectWithinAbsoluteError(controller.getOutputGainDb(), -3.0f, 0.001f);
        controller.setEffect(DfxDsp::Fidelity, std::numeric_limits<float>::infinity());
        controller.setEqBand(1000000, 10.0f);
        expectEquals(controller.getEqBand(-1), 0.0f);

        beginTest("oversized blocks preserve the result of prepared-size chunks");
        LegacyDspAdapter whole, pieces;
        expect(whole.prepare(48000, 64));
        expect(pieces.prepare(48000, 64));
        whole.setPowerOn(true); pieces.setPowerOn(true);
        whole.setEffectValue(DfxDsp::Bass, 0.5f); pieces.setEffectValue(DfxDsp::Bass, 0.5f);
        juce::AudioBuffer<float> a(2, 1024), b(2, 1024);
        fillTone(a); b.makeCopyOf(a);
        whole.process(a);
        for (int offset = 0; offset < 1024; offset += 64) {
            float* channels[] { b.getWritePointer(0, offset), b.getWritePointer(1, offset) };
            juce::AudioBuffer<float> part(channels, 2, 64);
            pieces.process(part);
        }
        expect(legal(a));
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 1024; ++i)
                expectWithinAbsoluteError(a.getSample(ch, i), b.getSample(ch, i), 0.0001f);

        beginTest("nonfinite input cannot poison subsequent audio");
        juce::AudioBuffer<float> bad(2, 256);
        fillTone(bad);
        bad.setSample(0, 1, std::numeric_limits<float>::quiet_NaN());
        bad.setSample(1, 8, std::numeric_limits<float>::infinity());
        controller.processBlock(bad); expect(legal(bad));
        fillTone(bad); controller.processBlock(bad); expect(legal(bad));
        controller.setBypassed(true);
        bad.setSample(0, 0, std::numeric_limits<float>::quiet_NaN());
        controller.processBlock(bad); expect(legal(bad));
        controller.setBypassed(false);

        beginTest("audio stays finite while the UI edits the same DSP");
        std::atomic<bool> good { true };
        std::thread audio([&] {
            juce::AudioBuffer<float> block(2, 64);
            for (int i = 0; i < 2500; ++i) {
                fillTone(block); controller.processBlock(block);
                if (!legal(block)) good = false;
            }
        });
        for (int i = 0; i < 800; ++i) {
            controller.setPower(i % 2 == 0);
            controller.setEffect(DfxDsp::Fidelity, (float)(i % 10) / 10.0f);
            controller.setEqBand(0, (float)(i % 12) - 6.0f);
            controller.setOutputGainDb((float)(i % 6) - 3.0f);
            (void)controller.getEffect(DfxDsp::Fidelity);
        }
        audio.join(); expect(good.load());

        beginTest("original factory presets produce non-silent finite audio");
        const juce::File presets(FXMAC_TEST_PRESET_DIR);
        for (const auto& preset : presets.findChildFiles(juce::File::findFiles, false, "*.fac")) {
            FxController processor;
            expect(processor.prepare(48000, 256));
            const bool loaded = processor.loadPreset(preset);
            expect(loaded, preset.getFileName());
            processor.setPower(true);
            juce::AudioBuffer<float> block(2, 256);
            double energy = 0;
            for (int i = 0; i < 16; ++i) {
                fillTone(block); processor.processBlock(block);
                expect(legal(block), preset.getFileName());
                energy += block.getRMSLevel(0, 0, block.getNumSamples());
            }
            expect(energy > 0.001, preset.getFileName() + " must not produce silence");
        }

        beginTest("custom presets round-trip, including Unicode names");
        const auto presetTemp = juce::File::getCurrentWorkingDirectory().getNonexistentChildFile("preset-roundtrip", "", false);
        expect(presetTemp.createDirectory().wasOk());
        for (const auto& name : juce::StringArray {"Evening", juce::String::fromUTF8("音楽")}) {
            FxController saved, loaded;
            expect(saved.prepare(48000, 256)); expect(loaded.prepare(48000, 256));
            saved.setEffect(DfxDsp::Fidelity, 0.70f); saved.setEqBand(0, 4.0f);
            const bool wrote = saved.savePreset(presetTemp, name);
            expect(wrote, name + " save");
            const auto file = presetTemp.getChildFile(name + ".fac");
            expect(file.existsAsFile(), name + " file");
            if (wrote && file.existsAsFile()) {
                expect(loaded.loadPreset(file), name + " load");
                expectWithinAbsoluteError(loaded.getEffect(DfxDsp::Fidelity), saved.getEffect(DfxDsp::Fidelity), 0.05f);
                expectWithinAbsoluteError(loaded.getEqBand(0), saved.getEqBand(0), 0.02f);
            }
        }
        presetTemp.deleteRecursively();

        const auto temp = juce::File::getCurrentWorkingDirectory().getNonexistentChildFile("routing-test", "", false);
        const auto journal = temp.getChildFile("lease.json");
        beginTest("stop restores the original output; manual changes win");
        FakeRouting backend;
        AudioRouteLease lease(backend, journal);
        expect(lease.begin("capture", "speakers")); expect(backend.current == "capture");
        expect(lease.end()); expect(backend.current == "speakers"); expect(!journal.existsAsFile());
        expect(lease.begin("capture", "speakers")); backend.current = "headphones";
        expect(lease.end()); expect(backend.current == "headphones");

        beginTest("failed starts and feedback routes never replace system output");
        backend.current = "speakers"; backend.failWrites = true;
        expect(!lease.begin("capture", "speakers")); expect(backend.current == "speakers");
        backend.failWrites = false;
        expect(!lease.begin("capture", "capture")); expect(backend.current == "speakers");

        beginTest("crash journal recovers the route on the next launch");
        expect(lease.begin("capture", "speakers"));
        AudioRouteLease recovered(backend, journal);
        expect(recovered.recover()); expect(backend.current == "speakers"); expect(!journal.existsAsFile());
        expect(lease.end());

        beginTest("unplugged headphones fall back to connected speakers");
        backend.current = "headphones";
        expect(lease.begin("capture", "headphones")); backend.connected.removeString("headphones");
        expect(lease.end()); expect(backend.current == "speakers");

        beginTest("failed restoration retains the recovery journal for retry");
        expect(lease.begin("capture", "speakers")); backend.failWrites = true;
        expect(!lease.end()); expect(journal.existsAsFile()); expect(lease.isActive());
        backend.failWrites = false; expect(lease.end()); expect(!journal.existsAsFile());
        temp.deleteRecursively();
    }
};
static ReliabilityTests reliabilityTests;
