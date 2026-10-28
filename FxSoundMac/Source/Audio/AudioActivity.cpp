// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include "AudioActivity.h"
#include <cmath>
#include <algorithm>

AudioSpectrum::AudioSpectrum()
{
    for (int i = 0; i < fftSize; ++i)
        window[(size_t)i] = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * (float)i / (float)(fftSize - 1));
}
float AudioSpectrum::bandFrequency(int band)
{
    return 20.0f * std::pow(1000.0f, (float)band / (float)(bandCount - 1));
}
void AudioSpectrum::reset()
{
    left.fill(0); right.fill(0); target.fill(0); display.fill(0);
    filled = 0; audioHold = 0; spectrumAge = 0;
}
void AudioSpectrum::update(AudioActivityTap& tap, double elapsedSeconds, bool analyseSpectrum)
{
    const double dt = juce::jlimit(0.001, 0.2, elapsedSeconds);
    if (generation != tap.getGeneration()) { reset(); generation = tap.getGeneration(); }
    audioHold = juce::jmax(0.0, audioHold - dt);
    spectrumAge += dt;
    bool transformed = false;
    float peak = 0;
    tap.consume([&](AudioActivityTap::Frame frame) {
        const float l = std::isfinite(frame.left) ? frame.left : 0;
        const float r = std::isfinite(frame.right) ? frame.right : 0;
        peak = juce::jmax(peak, std::abs(l), std::abs(r));
        if (!analyseSpectrum) return;
        left[(size_t)filled] = l; right[(size_t)filled] = r;
        if (++filled == fftSize)
        {
            if (!transformed) target.fill(0);
            transform(tap.getSampleRate());
            transformed = true; spectrumAge = 0;
            std::copy(left.begin() + fftSize / 2, left.end(), left.begin());
            std::copy(right.begin() + fftSize / 2, right.end(), right.begin());
            filled = fftSize / 2;
        }
    });
    // A short hold prevents the label flickering between words and quiet gaps.
    if (peak > 0.0001f) audioHold = 0.6;
    if (!analyseSpectrum) { target.fill(0); display.fill(0); filled = 0; return; }
    if (spectrumAge > 0.15) target.fill(0);
    for (int i = 0; i < bandCount; ++i)
    {
        auto& value = display[(size_t)i];
        const float desired = target[(size_t)i];
        const double time = desired > value ? 0.045 : 0.28;
        value += (desired - value) * (float)(1.0 - std::exp(-dt / time));
        if (value < 0.001f) value = 0;
    }
}
void AudioSpectrum::transform(double rate)
{
    if (!std::isfinite(rate) || rate < 8000) return;
    fftLeft.fill(0); fftRight.fill(0);
    for (int i = 0; i < fftSize; ++i) {
        fftLeft[(size_t)i] = left[(size_t)i] * window[(size_t)i];
        fftRight[(size_t)i] = right[(size_t)i] * window[(size_t)i];
    }
    fft.performFrequencyOnlyForwardTransform(fftLeft.data());
    fft.performFrequencyOnlyForwardTransform(fftRight.data());
    const float halfWidth = std::pow(1000.0f, 0.5f / (float)(bandCount - 1));
    for (int band = 0; band < bandCount; ++band)
    {
        const float centre = bandFrequency(band);
        if (centre > rate * 0.5) { target[(size_t)band] = 0; continue; }
        const int lo = juce::jlimit(1, fftSize / 2, (int)std::round(centre / halfWidth * fftSize / rate));
        const int hi = juce::jlimit(lo, fftSize / 2, (int)std::round(centre * halfWidth * fftSize / rate));
        float magnitude = 0;
        for (int bin = lo; bin <= hi; ++bin)
        {
            const auto l = fftLeft[(size_t)bin], r = fftRight[(size_t)bin];
            magnitude = juce::jmax(magnitude, std::sqrt((l * l + r * r) * 0.5f) * (4.0f / fftSize));
        }
        const float db = juce::Decibels::gainToDecibels(magnitude, -72.0f);
        target[(size_t)band] = juce::jmax(target[(size_t)band], juce::jlimit(0.0f, 1.0f, (db + 72.0f) / 72.0f));
    }
}
