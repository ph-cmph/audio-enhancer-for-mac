// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include <functional>
#include "AudioEngineStatus.h"
#include "AudioRouteLease.h"
#include "SystemVolumeBridge.h"
#include "AudioActivity.h"
#include "../Support/FxController.h"

class MacAudioEngine : private juce::AudioIODeviceCallback, private juce::Timer
{
public:
    explicit MacAudioEngine(FxController&);
    ~MacAudioEngine() override;
    juce::StringArray getInputDeviceNames() const;
    juce::StringArray getOutputDeviceNames() const;
    AudioEngineStatus start(const juce::String& outputDeviceName);
    void stop();
    bool isRunning() const { return running.load(); }
    AudioEngineStatus setOutputDevice(const juce::String& outputDeviceName);
    AudioEngineStatus getStatus() const { return status; }
    AudioActivityTap& activityTap() { return activity; }
    std::function<void(AudioEngineStatus)> onStatusChanged; // message thread only
private:
    void audioDeviceIOCallbackWithContext(const float* const*, int, float* const*, int, int,
                                          const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart(juce::AudioIODevice*) override;
    void audioDeviceStopped() override;
    void timerCallback() override;
    void setStatus(AudioEngineStatus);
    juce::AudioDeviceManager deviceManager;
    FxController& controller;
    juce::AudioBuffer<float> scratch;
    AudioActivityTap activity;
    CoreAudioRoutingBackend routeBackend;
    AudioRouteLease route;
    CoreAudioVolumeBackend volumeBackend;
    SystemVolumeBridge volumeBridge;
    std::atomic<float> inputCompensation { 1.0f };
    AudioEngineStatus status { AudioEngineState::Stopped, {} };
    std::atomic<bool> running { false }, starting { false }, dspReady { false }, deviceChanged { false };
    bool callbackAttached = false;
};
