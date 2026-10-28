// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

// Independent Mac preview changes, 2026-10-28, AGPL-3.0-or-later.
// One audio writer, one UI reader. Full queue drops visualisation frames only.
// Audio samples are observed, never altered or written to disk.
class AudioActivityTap
{
public:
    static constexpr int capacity = 8192;
    struct Frame { float left = 0, right = 0; };
    void begin(double rate) noexcept { sampleRate.store(rate); generation.fetch_add(1); }
    double getSampleRate() const noexcept { return sampleRate.load(); }
    unsigned getGeneration() const noexcept { return generation.load(); }
    void push(const juce::AudioBuffer<float>& audio) noexcept
    {
        if (audio.getNumChannels() <= 0 || audio.getNumSamples() <= 0) return;
        int a, na, b, nb;
        fifo.prepareToWrite(audio.getNumSamples(), a, na, b, nb);
        const auto* left = audio.getReadPointer(0);
        const auto* right = audio.getReadPointer(audio.getNumChannels() > 1 ? 1 : 0);
        for (int i = 0; i < na; ++i) frames[(size_t)(a + i)] = {left[i], right[i]};
        for (int i = 0; i < nb; ++i) frames[(size_t)(b + i)] = {left[na + i], right[na + i]};
        fifo.finishedWrite(na + nb);
    }
    template <typename Reader> void consume(Reader&& reader)
    {
        int a, na, b, nb;
        fifo.prepareToRead(fifo.getNumReady(), a, na, b, nb);
        for (int i = 0; i < na; ++i) reader(frames[(size_t)(a + i)]);
        for (int i = 0; i < nb; ++i) reader(frames[(size_t)(b + i)]);
        fifo.finishedRead(na + nb);
    }
    void discard() { fifo.finishedRead(fifo.getNumReady()); }
private:
    juce::AbstractFifo fifo {capacity};
    std::array<Frame, capacity> frames {};
    std::atomic<double> sampleRate {48000.0};
    std::atomic<unsigned> generation {0};
};

// FFT and smoothing run exclusively on the UI thread, away from playback.
class AudioSpectrum
{
public:
    static constexpr int order = 11, fftSize = 1 << order, bandCount = 48;
    AudioSpectrum();
    void reset();
    void update(AudioActivityTap&, double elapsedSeconds, bool analyseSpectrum = true);
    bool hasAudio() const { return audioHold > 0; }
    const std::array<float, bandCount>& levels() const { return display; }
    static float bandFrequency(int band);
private:
    void transform(double sampleRate);
    juce::dsp::FFT fft {order};
    std::array<float, fftSize> left {}, right {}, window {};
    std::array<float, fftSize * 2> fftLeft {}, fftRight {};
    std::array<float, bandCount> target {}, display {};
    int filled = 0;
    unsigned generation = 0;
    double audioHold = 0;
    double spectrumAge = 0;
};
