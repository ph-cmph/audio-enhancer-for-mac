// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include "FxController.h"

bool FxController::prepare(int rate, int frames)
{
    std::lock_guard<std::mutex> lock(dspGate);
    prepared = adapter.prepare(rate, frames);
    return prepared;
}
void FxController::setPower(bool on) { std::lock_guard<std::mutex> lock(dspGate); adapter.setPowerOn(on); }
bool FxController::isPowerOn() const { std::lock_guard<std::mutex> lock(dspGate); return adapter.isPowerOn(); }
void FxController::setBypassed(bool b) { adapter.setBypassed(b); }
void FxController::setEffect(DfxDsp::Effect e, float v) { std::lock_guard<std::mutex> lock(dspGate); adapter.setEffectValue(e, v); }
float FxController::getEffect(DfxDsp::Effect e) const { std::lock_guard<std::mutex> lock(dspGate); return adapter.getEffectValue(e); }
void FxController::setEqBand(int b, float db) { std::lock_guard<std::mutex> lock(dspGate); adapter.setEqBandBoostCut(b, db); }
float FxController::getEqBand(int b) const { std::lock_guard<std::mutex> lock(dspGate); return adapter.getEqBandBoostCut(b); }
int FxController::getNumEqBands() const { std::lock_guard<std::mutex> lock(dspGate); return adapter.getNumEqBands(); }
float FxController::getEqBandFrequency(int b) const { std::lock_guard<std::mutex> lock(dspGate); return adapter.getEqBandFrequency(b); }
void FxController::setOutputGainDb(float db) { adapter.setOutputGainDb(db); }
float FxController::getOutputGainDb() const { return adapter.getOutputGainDb(); }
bool FxController::loadPreset(const juce::File& file) { std::lock_guard<std::mutex> lock(dspGate); return adapter.loadPreset(file); }
bool FxController::savePreset(const juce::File& dir, const juce::String& name) { std::lock_guard<std::mutex> lock(dspGate); return adapter.savePreset(dir, name); }
void FxController::processBlock(juce::AudioBuffer<float>& buffer)
{
    std::unique_lock<std::mutex> lock(dspGate, std::try_to_lock);
    if (lock.owns_lock() && prepared) adapter.process(buffer);
    else adapter.processDry(buffer); // bounded, allocation-free pass-through during edits
}
