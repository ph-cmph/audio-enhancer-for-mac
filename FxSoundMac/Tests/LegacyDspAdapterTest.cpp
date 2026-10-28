// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "LegacyDspAdapter.h"
#include <cmath>

struct LegacyDspAdapterTest : juce::UnitTest
{
    LegacyDspAdapterTest() : juce::UnitTest("LegacyDspAdapter") {}
    void runTest() override
    {
        LegacyDspAdapter adapter;
        adapter.prepare(48000, 512);

        beginTest("effect value is clamped to [0,1]");
        adapter.setEffectValue(DfxDsp::Fidelity, 5.0f);
        expect(adapter.getEffectValue(DfxDsp::Fidelity) <= 1.0f);
        adapter.setEffectValue(DfxDsp::Fidelity, -5.0f);
        expect(adapter.getEffectValue(DfxDsp::Fidelity) >= 0.0f);

        beginTest("output gain in dB clamps to supported range");
        adapter.setOutputGainDb(999.0f);
        expect(adapter.getOutputGainDb() <= LegacyDspAdapter::maxOutputGainDb);
        adapter.setOutputGainDb(-999.0f);
        expect(adapter.getOutputGainDb() >= LegacyDspAdapter::minOutputGainDb);

        beginTest("bypass passes audio through unchanged");
        juce::AudioBuffer<float> buf(2, 64);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 64; ++i)
                buf.setSample(ch, i, 0.5f);
        adapter.setBypassed(true);
        adapter.setOutputGainDb(0.0f);
        adapter.process(buf);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 64; ++i)
                expectWithinAbsoluteError(buf.getSample(ch, i), 0.5f, 1.0e-4f);

        beginTest("unity output gain linear value is 1.0");
        adapter.setOutputGainDb(0.0f);
        expectWithinAbsoluteError(adapter.linearOutputGain(), 1.0f, 1.0e-6f);

        beginTest("measure dynamic boost at quiet input levels");
        double gains[3] {}; int level = 0;
        for (float amount : {0.0f, 0.5f, 1.0f})
        {
            LegacyDspAdapter probe;
            expect(probe.prepare(48000, 512));
            probe.setPowerOn(true);
            probe.setEffectValue(DfxDsp::DynamicBoost, amount);
            double inputEnergy = 0, outputEnergy = 0;
            juce::AudioBuffer<float> audio(2, 512);
            for (int block = 0; block < 300; ++block)
            {
                for (int i = 0; i < 512; ++i)
                {
                    float value = 0.01f * std::sin(juce::MathConstants<double>::twoPi * 997.0 * (block * 512 + i) / 48000.0);
                    audio.setSample(0, i, value); audio.setSample(1, i, value);
                    if (block >= 100) inputEnergy += 2.0 * value * value;
                }
                probe.process(audio);
                if (block >= 100)
                    for (int ch = 0; ch < 2; ++ch)
                        for (int i = 0; i < 512; ++i) outputEnergy += std::pow(audio.getSample(ch, i), 2);
            }
            gains[level++] = 10.0 * std::log10(outputEnergy / inputEnergy);
            logMessage("Dynamic Boost " + juce::String(amount) + ": " + juce::String(gains[level - 1], 2) + " dB");
        }
        expectWithinAbsoluteError(gains[0], 0.0, 0.5);
        expect(gains[1] > 8.0 && gains[1] < 12.0, "Midpoint boost should raise a quiet signal by roughly 10 dB");
        expect(gains[2] > gains[1] + 0.5, "Maximum boost should increase level further");
    }
};
static LegacyDspAdapterTest legacyDspAdapterTest;
