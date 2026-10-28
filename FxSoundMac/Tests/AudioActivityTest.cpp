// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include <juce_core/juce_core.h>
#include "AudioActivity.h"
#include <cmath>
#include <limits>
#include <algorithm>

struct AudioActivityTest : juce::UnitTest
{
    AudioActivityTest() : juce::UnitTest("Audio activity") {}
    void runTest() override
    {
        beginTest("silence has no animated or active signal");
        AudioActivityTap tap;
        AudioSpectrum spectrum;
        juce::AudioBuffer<float> block(2, 512); block.clear();
        tap.begin(48000);
        for (int i = 0; i < 12; ++i) { tap.push(block); spectrum.update(tap, 0.033); }
        expect(!spectrum.hasAudio());
        for (float value : spectrum.levels()) expectEquals(value, 0.0f);

        beginTest("actual tone controls the spectrum at supported sample rates");
        for (double rate : {8000.0, 44100.0, 48000.0, 96000.0, 192000.0})
        {
            tap.begin(rate); spectrum.reset(); tap.discard();
            for (int n = 0; n < 60; ++n)
            {
                for (int i = 0; i < 512; ++i)
                {
                    const float value = 0.1f * std::sin((float)(juce::MathConstants<double>::twoPi * 997.0 * (n * 512 + i) / rate));
                    block.setSample(0, i, value); block.setSample(1, i, -value); // opposite stereo phase must not cancel
                }
                const float original = block.getSample(0, 30);
                tap.push(block);
                expectEquals(block.getSample(0, 30), original, "Tap must not modify playback samples");
                spectrum.update(tap, 0.02);
            }
            expect(spectrum.hasAudio());
            const auto strongest = std::max_element(spectrum.levels().begin(), spectrum.levels().end());
            const int band = (int)std::distance(spectrum.levels().begin(), strongest);
            expect(*strongest > 0.4f);
            expect(std::abs(AudioSpectrum::bandFrequency(band) - 997.0f) < 250.0f);
        }

        beginTest("silence releases the display and then the activity label");
        block.clear();
        for (int i = 0; i < 150; ++i) { tap.push(block); spectrum.update(tap, 0.033); }
        expect(!spectrum.hasAudio());
        for (float value : spectrum.levels()) expectEquals(value, 0.0f);

        beginTest("right-only audio is detected; reduced motion needs no animated spectrum");
        for (int i = 0; i < 512; ++i) block.setSample(1, i, 0.1f * std::sin((float)i * 0.17f));
        tap.push(block); spectrum.update(tap, 0.033, false);
        expect(spectrum.hasAudio());
        for (float value : spectrum.levels()) expectEquals(value, 0.0f);

        beginTest("stopping clears history; invalid samples cannot invent activity");
        spectrum.reset(); tap.discard();
        for (int i = 0; i < 512; ++i) {
            block.setSample(0, i, std::numeric_limits<float>::quiet_NaN());
            block.setSample(1, i, std::numeric_limits<float>::infinity());
        }
        tap.push(block); spectrum.update(tap, 0.033);
        expect(!spectrum.hasAudio());
        for (float value : spectrum.levels()) expectEquals(value, 0.0f);

        beginTest("a hidden UI cannot block or grow the audio queue");
        tap.discard(); block.clear();
        for (int n = 0; n < 40; ++n) tap.push(block);
        int consumed = 0; tap.consume([&](AudioActivityTap::Frame) { ++consumed; });
        expectEquals(consumed, AudioActivityTap::capacity - 1);
        tap.push(block); consumed = 0;
        tap.consume([&](AudioActivityTap::Frame) { ++consumed; });
        expectEquals(consumed, 512);
        juce::AudioBuffer<float> empty; tap.push(empty);
    }
};
static AudioActivityTest audioActivityTest;
