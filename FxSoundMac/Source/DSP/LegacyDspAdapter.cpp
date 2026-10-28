// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include "LegacyDspAdapter.h"
#include <cmath>

LegacyDspAdapter::LegacyDspAdapter() = default;
LegacyDspAdapter::~LegacyDspAdapter() = default;

bool LegacyDspAdapter::prepare(int sampleRate, int maxBlockSize)
{
    if (sampleRate < 8000 || sampleRate > 192000 || maxBlockSize <= 0 || maxBlockSize > 65536)
        return false;
    currentSampleRate = sampleRate;
    allocatedFrames = juce::jmax(maxBlockSize, 1);
    inFloat.calloc((size_t) allocatedFrames * 2);
    outFloat.calloc((size_t) allocatedFrames * 2);

    // The DFX DSP build (PT_DSP_BUILD=PT_DSP_DFX, DSPSOFT_32_BIT) processes audio
    // natively as 32-bit float (COM_32_BIT_FLOAT_SAMPLES). Feeding int16 forced the
    // DSP to do float→int16→float internally, and the adapter added another int16
    // round trip — two quantization stages whose noise the effects (esp. the Aural
    // exciter) amplify into a gritty "cheap speaker" sound. Use 32-bit float so audio
    // stays float end to end, matching the Windows signal path. No quantization.
    const bool ok = dsp.setSignalFormat(32, 2, sampleRate, 32) == 0;

    // setSignalFormat()/dfxpBeginProcess() re-initialises the DSP from defaults
    // (filter coefficients from non-zero MIDI defaults; effect knobs reset). Since
    // (re)starting the audio device re-runs prepare(), we must re-apply the user's
    // desired control state here — otherwise effects/EQ silently revert until a
    // slider is moved. On first launch the caches are all zero, which also matches
    // the UI (sliders start at 0) and avoids the "processed even at zero" mismatch.
    if (ok)
        reapplyState();

    return ok;
}

void LegacyDspAdapter::reapplyState()
{
    dsp.powerOn(powerCache_);

    for (int e = 0; e < static_cast<int>(DfxDsp::NumEffects); ++e)
        dsp.setEffectValue(static_cast<DfxDsp::Effect>(e), effectCache_[e]);

    const int numBands = dsp.getNumEqBands();
    if (static_cast<int>(eqCache_.size()) < numBands)
        eqCache_.resize(static_cast<size_t>(numBands), 0.0f);
    for (int b = 0; b < numBands; ++b)
        dsp.setEqBandBoostCut(b, eqCache_[b]);
}

void LegacyDspAdapter::setPowerOn(bool on) { powerCache_ = on; dsp.powerOn(on); }
bool LegacyDspAdapter::isPowerOn() const { return const_cast<DfxDsp&>(dsp).isPowerOn(); }
void LegacyDspAdapter::setBypassed(bool b) { bypassed.store(b); }
bool LegacyDspAdapter::isBypassed() const { return bypassed.load(); }

void LegacyDspAdapter::setMasterGainDb(float db) { if (std::isfinite(db)) dsp.setMasterGain(juce::jlimit(-60.0f, 12.0f, db)); }
float LegacyDspAdapter::getMasterGainDb() const { return const_cast<DfxDsp&>(dsp).getMasterGain(); }

void LegacyDspAdapter::setOutputGainDb(float db)
{
    if (std::isfinite(db)) outputGainDb.store(juce::jlimit(minOutputGainDb, maxOutputGainDb, db));
}
float LegacyDspAdapter::getOutputGainDb() const { return outputGainDb.load(); }
float LegacyDspAdapter::linearOutputGain() const
{
    return juce::Decibels::decibelsToGain(outputGainDb.load(), minOutputGainDb);
}

void LegacyDspAdapter::setEffectValue(DfxDsp::Effect e, float v)
{
    if (!std::isfinite(v) || static_cast<int>(e) < 0 || static_cast<int>(e) >= static_cast<int>(DfxDsp::NumEffects)) return;
    const float clamped = juce::jlimit(0.0f, 1.0f, v);
    const int idx = static_cast<int>(e);
    if (idx >= 0 && idx < static_cast<int>(DfxDsp::NumEffects))
        effectCache_[idx] = clamped;
    dsp.setEffectValue(e, clamped);
}
float LegacyDspAdapter::getEffectValue(DfxDsp::Effect e) const
{
    return const_cast<DfxDsp&>(dsp).getEffectValue(e);
}

int LegacyDspAdapter::getNumEqBands() const { return const_cast<DfxDsp&>(dsp).getNumEqBands(); }
float LegacyDspAdapter::getEqBandFrequency(int band) const { return band >= 0 && band < getNumEqBands() ? const_cast<DfxDsp&>(dsp).getEqBandFrequency(band) : 0.0f; }
void LegacyDspAdapter::setEqBandBoostCut(int band, float db)
{
    if (band < 0 || band >= dsp.getNumEqBands() || !std::isfinite(db)) return;
    db = juce::jlimit(-12.0f, 12.0f, db);
    if (band >= 0)
    {
        if (band >= static_cast<int>(eqCache_.size()))
            eqCache_.resize(static_cast<size_t>(band) + 1, 0.0f);
        eqCache_[static_cast<size_t>(band)] = db;
    }
    dsp.setEqBandBoostCut(band, db);
}
float LegacyDspAdapter::getEqBandBoostCut(int band) const { return band >= 0 && band < getNumEqBands() ? const_cast<DfxDsp&>(dsp).getEqBandBoostCut(band) : 0.0f; }

bool LegacyDspAdapter::loadPreset(const juce::File& presetFile)
{
    if (! presetFile.existsAsFile())
        return false;
    std::wstring path = presetFile.getFullPathName().toWideCharPointer();
    const bool ok = dsp.loadPreset(path) == 0; // OKAY == 0

    // Sync caches from the DSP so a later prepare()/restart re-applies the preset
    // (not the pre-preset values). loadPreset writes the new state into the DSP;
    // read it back here as the new "desired" state.
    if (ok)
    {
        for (int e = 0; e < static_cast<int>(DfxDsp::NumEffects); ++e)
            effectCache_[e] = dsp.getEffectValue(static_cast<DfxDsp::Effect>(e));

        const int numBands = dsp.getNumEqBands();
        eqCache_.assign(static_cast<size_t>(numBands), 0.0f);
        for (int b = 0; b < numBands; ++b)
            eqCache_[static_cast<size_t>(b)] = dsp.getEqBandBoostCut(b);
    }
    return ok;
}

bool LegacyDspAdapter::savePreset(const juce::File& directory, const juce::String& name)
{
    // DfxDsp::savePreset takes (preset_name, directory_path); it appends ".fac"
    // to the name and writes the current DSP state (effects + EQ) into directory.
    std::wstring dir   = directory.getFullPathName().toWideCharPointer();
    std::wstring wname = name.toWideCharPointer();
    return dsp.savePreset(wname, dir) == 0; // OKAY == 0
}

namespace {
float legalSample(float value) noexcept
{
    return std::isfinite(value) ? juce::jlimit(-1.0f, 1.0f, value) : 0.0f;
}
}

void LegacyDspAdapter::processDry(juce::AudioBuffer<float>& buffer) const
{
    const float gain = linearOutputGain();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample(ch, i, legalSample(legalSample(buffer.getSample(ch, i)) * gain));
}

void LegacyDspAdapter::process(juce::AudioBuffer<float>& buffer)
{
    const int frames = buffer.getNumSamples();
    const int channels = buffer.getNumChannels();
    if (frames <= 0 || channels <= 0) return;
    if (bypassed.load() || allocatedFrames <= 0) { processDry(buffer); return; }
    const float gain = linearOutputGain();
    const float* left = buffer.getReadPointer(0);
    const float* right = channels > 1 ? buffer.getReadPointer(1) : left;
    // An unexpectedly large callback is split into prepared chunks. Never allocate
    // or reinitialise the DSP in the audio callback; preserve its filter history.
    for (int offset = 0; offset < frames; offset += allocatedFrames)
    {
        const int count = juce::jmin(allocatedFrames, frames - offset);
        for (int i = 0; i < count; ++i)
        {
            inFloat[(size_t)i * 2] = legalSample(left[offset + i]);
            inFloat[(size_t)i * 2 + 1] = legalSample(right[offset + i]);
        }
        const bool ok = dsp.processAudio(reinterpret_cast<short*>(inFloat.getData()),
                                        reinterpret_cast<short*>(outFloat.getData()), count, 0) == 0;
        for (int ch = 0; ch < channels; ++ch)
        {
            float* output = buffer.getWritePointer(ch) + offset;
            const int source = juce::jmin(ch, 1);
            for (int i = 0; i < count; ++i)
                output[i] = legalSample((ok ? outFloat[(size_t)i * 2 + source]
                                           : inFloat[(size_t)i * 2 + source]) * gain);
        }
    }
}
