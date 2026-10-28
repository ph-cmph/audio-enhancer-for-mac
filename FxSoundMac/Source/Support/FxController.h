// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <mutex>
#include "LegacyDspAdapter.h"

// Modified for the independent macOS reliability build, 2026-10-28.
// Every DSP access shares one gate. The audio callback only tries the gate:
// a preset load/control edit can never block playback or race the DSP.
class FxController
{
public:
    bool prepare(int sampleRate, int maxBlockSize);
    void setPower(bool on);
    bool isPowerOn() const;
    void setBypassed(bool b);
    void setEffect(DfxDsp::Effect e, float value);
    float getEffect(DfxDsp::Effect e) const;
    void setEqBand(int band, float db);
    float getEqBand(int band) const;
    int getNumEqBands() const;
    float getEqBandFrequency(int band) const;
    void setOutputGainDb(float db);
    float getOutputGainDb() const;
    bool loadPreset(const juce::File& presetFile);
    bool savePreset(const juce::File& directory, const juce::String& name);
    void processBlock(juce::AudioBuffer<float>& buffer);
private:
    LegacyDspAdapter adapter;
    mutable std::mutex dspGate;
    bool prepared = false; // guarded by dspGate
};
