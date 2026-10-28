// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include "MacAudioEngine.h"
#include "DeviceValidation.h"
#include "../Support/AppSettings.h"

MacAudioEngine::MacAudioEngine(FxController& c)
    : controller(c), route(routeBackend, AppSettings::settingsDir().getChildFile("audio-route.json")),
      volumeBridge(volumeBackend, AppSettings::settingsDir().getChildFile("audio-volume.json"))
{
    const bool volumeRecovered = volumeBridge.recover();
    const bool routeRecovered = route.recover();
    if (!volumeRecovered || !routeRecovered) status = { AudioEngineState::RestoreFailed, {} };
    startTimer(50);
}
MacAudioEngine::~MacAudioEngine() { stopTimer(); onStatusChanged = {}; stop(); }

static juce::StringArray deviceNamesForType(juce::AudioDeviceManager& manager, bool inputs)
{
    juce::StringArray names;
    for (auto* type : manager.getAvailableDeviceTypes())
    {
        type->scanForDevices();
        names.addArray(type->getDeviceNames(inputs));
    }
    names.removeDuplicates(false);
    return names;
}
juce::StringArray MacAudioEngine::getInputDeviceNames() const { return deviceNamesForType(const_cast<juce::AudioDeviceManager&>(deviceManager), true); }
juce::StringArray MacAudioEngine::getOutputDeviceNames() const
{
    juce::StringArray outputs;
    for (const auto& name : deviceNamesForType(const_cast<juce::AudioDeviceManager&>(deviceManager), false))
        if (routeBackend.isPhysicalOutput(name)) outputs.add(name);
    return outputs;
}

AudioEngineStatus MacAudioEngine::start(const juce::String& output)
{
    stop();
    if (route.isActive()) { setStatus({AudioEngineState::RestoreFailed, {}}); return status; }
    const auto capture = DeviceValidation::findBlackHoleDevice(getInputDeviceNames());
    const auto failure = DeviceValidation::validateRoute(capture, output, getOutputDeviceNames());
    if (failure.state != AudioEngineState::Stopped) { setStatus(failure); return status; }
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    setup.inputDeviceName = capture;
    setup.outputDeviceName = output;
    setup.useDefaultInputChannels = false;
    setup.useDefaultOutputChannels = false;
    setup.inputChannels.setRange(0, 2, true);
    setup.outputChannels.setRange(0, 2, true);
    // Never fall back to a physical microphone or another device if the requested route fails.
    const auto error = deviceManager.initialise(2, 2, nullptr, false, {}, &setup);
    if (error.isNotEmpty())
    {
        deviceManager.closeAudioDevice();
        setStatus({AudioEngineState::EngineFailedToStart, error});
        return status;
    }
    auto* device = deviceManager.getCurrentAudioDevice();
    if (!device || device->getActiveInputChannels().countNumberOfSetBits() != 2
                || device->getActiveOutputChannels().countNumberOfSetBits() != 2)
    {
        deviceManager.closeAudioDevice();
        setStatus({AudioEngineState::UnsupportedChannelLayout, {}});
        return status;
    }
    if (!volumeBridge.begin(routeBackend.uidForName(capture), routeBackend.uidForName(output)))
    {
        stop();
        setStatus({AudioEngineState::VolumeSetupFailed, {}});
        return status;
    }
    inputCompensation.store(volumeBridge.inputCompensation());
    dspReady = false;
    deviceChanged = false;
    starting = true;
    deviceManager.addAudioCallback(this);
    callbackAttached = true;
    starting = false;
    if (!dspReady.load())
    {
        stop();
        setStatus({AudioEngineState::DspFailedToInitialize, {}});
        return status;
    }
    // Route system audio only after the complete processing path is ready.
    if (!route.begin(routeBackend.uidForName(capture), routeBackend.uidForName(output)))
    {
        stop();
        setStatus({AudioEngineState::RouteFailed, {}});
        return status;
    }
    running = true;
    setStatus({AudioEngineState::Running, output});
    return status;
}
void MacAudioEngine::stop()
{
    if (running.load()) (void)volumeBridge.tick();
    running = false;
    starting = false;
    if (callbackAttached) { deviceManager.removeAudioCallback(this); callbackAttached = false; }
    deviceManager.closeAudioDevice();
    dspReady = false;
    deviceChanged = false;
    const bool volumeRestored = volumeBridge.end();
    inputCompensation = 1.0f;
    const bool routeRestored = route.end();
    setStatus({volumeRestored && routeRestored ? AudioEngineState::Stopped : AudioEngineState::RestoreFailed, {}});
}
AudioEngineStatus MacAudioEngine::setOutputDevice(const juce::String& output)
{
    if (!isRunning()) return status;
    return start(output); // stop/restore first; a failed new device never leaves a live broken route
}
void MacAudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (!starting.load())
    {
        // A hot unplug, sleep/wake or OS format change must not silently reopen a microphone.
        dspReady = false;
        deviceChanged = true;
        return;
    }
    const int block = juce::jmax(1, device->getCurrentBufferSizeSamples());
    scratch.setSize(2, block, false, false, true);
    activity.begin(device->getCurrentSampleRate());
    dspReady = controller.prepare((int)device->getCurrentSampleRate(), block);
}
void MacAudioEngine::audioDeviceStopped() { dspReady = false; if (running.load()) deviceChanged = true; }
void MacAudioEngine::timerCallback()
{
    const bool changed = deviceChanged.exchange(false);
    if (running.load() && (changed || !dspReady.load()))
    {
        stop();
        if (status.state != AudioEngineState::RestoreFailed)
            setStatus({AudioEngineState::DeviceUnavailable, {}});
    }
    if (running.load())
    {
        if (!volumeBridge.tick())
        {
            stop();
            setStatus({AudioEngineState::VolumeSetupFailed, {}});
        }
        else inputCompensation.store(volumeBridge.inputCompensation());
    }
}
void MacAudioEngine::audioDeviceIOCallbackWithContext(const float* const* input, int inputChannels,
        float* const* output, int outputChannels, int frames, const juce::AudioIODeviceCallbackContext&)
{
    if (!dspReady.load() || scratch.getNumSamples() <= 0)
    {
        for (int ch = 0; ch < outputChannels; ++ch) if (output[ch]) juce::FloatVectorOperations::clear(output[ch], frames);
        return;
    }
    const int capacity = scratch.getNumSamples();
    for (int offset = 0; offset < frames; offset += capacity)
    {
        const int count = juce::jmin(capacity, frames - offset);
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* source = ch < inputChannels ? input[ch] : nullptr;
            if (source) scratch.copyFrom(ch, 0, source + offset, count);
            else scratch.clear(ch, 0, count);
        }
        juce::AudioBuffer<float> view(scratch.getArrayOfWritePointers(), 2, count);
        view.applyGain(inputCompensation.load());
        controller.processBlock(view);
        activity.push(view);
        for (int ch = 0; ch < outputChannels; ++ch)
            if (output[ch])
            {
                if (ch < 2) juce::FloatVectorOperations::copy(output[ch] + offset, scratch.getReadPointer(ch), count);
                else juce::FloatVectorOperations::clear(output[ch] + offset, count);
            }
    }
}
void MacAudioEngine::setStatus(AudioEngineStatus next)
{
    status = std::move(next);
    if (onStatusChanged) onStatusChanged(status);
}
