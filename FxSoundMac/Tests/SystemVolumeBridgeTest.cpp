// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include <juce_core/juce_core.h>
#include "SystemVolumeBridge.h"
#include "LegacyDspAdapter.h"
#include <map>

namespace {
struct FakeVolumes : VolumeBridgeBackend
{
    std::map<juce::String, DeviceVolume> values {
        {"capture", {0.625f, false, -24.0f, true}},
        {"speakers", {0.4356f, false, -34.0f, true}}
    };
    bool failWrites = false;
    bool readVolume(const juce::String& uid, DeviceVolume& v) const override
    {
        auto found = values.find(uid); if (found == values.end()) return false;
        v = found->second; return true;
    }
    bool writeVolume(const juce::String& uid, const DeviceVolume& v) override
    {
        if (failWrites || !values.count(uid)) return false;
        values[uid] = v;
        if (uid == "capture") { values[uid].decibels = 64.0f * (v.scalar - 1.0f); values[uid].hasDecibels = true; }
        return true;
    }
};
}
struct SystemVolumeBridgeTest : juce::UnitTest
{
    SystemVolumeBridgeTest() : juce::UnitTest("System volume bridge") {}
    void runTest() override
    {
        auto folder = juce::File::getCurrentWorkingDirectory().getNonexistentChildFile("volume-test", "", false);
        expect(folder.createDirectory().wasOk());
        const auto journal = folder.getChildFile("volume.json");
        FakeVolumes backend;
        SystemVolumeBridge bridge(backend, journal);
        beginTest("cancel virtual attenuation without raising physical volume");
        expect(bridge.begin("capture", "speakers"));
        expectWithinAbsoluteError(backend.values["speakers"].scalar, 0.4356f, 0.00001f);
        expectWithinAbsoluteError(backend.values["capture"].scalar, 0.4356f, 0.00001f);
        const auto virtualGain = std::pow(10.0f, backend.values["capture"].decibels / 20.0f);
        expectWithinAbsoluteError(virtualGain * bridge.inputCompensation(), 1.0f, 0.00001f);

        beginTest("Mac volume controls change the physical output once");
        backend.writeVolume("capture", {0.5f, false});
        expect(bridge.tick());
        expectWithinAbsoluteError(backend.values["speakers"].scalar, 0.5f, 0.00001f);
        backend.writeVolume("speakers", {0.3f, true});
        expect(bridge.tick());
        expectWithinAbsoluteError(backend.values["capture"].scalar, 0.3f, 0.00001f);
        expect(backend.values["capture"].muted);
        expectEquals(bridge.inputCompensation(), 0.0f);

        beginTest("stop restores virtual volume and retains the user's listening volume");
        expect(bridge.end());
        expectWithinAbsoluteError(backend.values["capture"].scalar, 0.625f, 0.00001f);
        expectWithinAbsoluteError(backend.values["speakers"].scalar, 0.3f, 0.00001f);
        expect(backend.values["speakers"].muted);
        expect(!journal.existsAsFile());

        beginTest("next launch recovers an interrupted volume session");
        expect(bridge.begin("capture", "speakers"));
        SystemVolumeBridge recovered(backend, journal);
        expect(recovered.recover());
        expectWithinAbsoluteError(backend.values["capture"].scalar, 0.625f, 0.00001f);

        beginTest("a later unrelated virtual-volume edit wins during recovery");
        SystemVolumeBridge owner(backend, journal);
        expect(owner.begin("capture", "speakers"));
        backend.writeVolume("capture", {0.8f, false});
        expect(recovered.recover());
        expectWithinAbsoluteError(backend.values["capture"].scalar, 0.8f, 0.00001f);

        beginTest("failed restoration keeps recovery information");
        SystemVolumeBridge failed(backend, journal);
        expect(failed.begin("capture", "speakers"));
        backend.failWrites = true;
        expect(!failed.end()); expect(journal.existsAsFile());
        backend.failWrites = false;
        expect(failed.recover()); expect(!journal.existsAsFile());

        beginTest("fixed-volume outputs are left alone");
        expect(failed.begin("capture", "fixed-level output"));
        expectEquals(failed.inputCompensation(), 1.0f);
        expect(!journal.existsAsFile());

        beginTest("real DSP receives the same signal after virtual attenuation correction");
        LegacyDspAdapter direct, throughVirtual;
        expect(direct.prepare(48000, 256)); expect(throughVirtual.prepare(48000, 256));
        direct.setEffectValue(DfxDsp::DynamicBoost, 0.5f);
        throughVirtual.setEffectValue(DfxDsp::DynamicBoost, 0.5f);
        const float attenuation = std::pow(10.0f, -24.0f / 20.0f);
        const float correction = SystemVolumeBridge::compensationFor({0.625f, false, -24.0f, true});
        for (int block = 0; block < 32; ++block)
        {
            juce::AudioBuffer<float> a(2, 256), b(2, 256);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 256; ++i) {
                    const float sample = 0.01f * std::sin((float)(block * 256 + i) * 0.13f);
                    a.setSample(ch, i, sample); b.setSample(ch, i, sample * attenuation);
                }
            b.applyGain(correction); direct.process(a); throughVirtual.process(b);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 256; ++i)
                    expectWithinAbsoluteError(a.getSample(ch, i), b.getSample(ch, i), 0.00001f);
        }
        expect(folder.deleteRecursively());
    }
};
static SystemVolumeBridgeTest systemVolumeBridgeTest;
