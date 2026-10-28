// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#pragma once
#include <juce_core/juce_core.h>

enum class AudioEngineState
{
    Stopped, RouteFailed, RestoreFailed, DeviceUnavailable, InvalidOutput, VolumeSetupFailed,
    BlackHoleNotInstalled,
    BlackHoleNotSelected,
    NoOutputSelected,
    SampleRateMismatch,
    UnsupportedChannelLayout,
    EngineFailedToStart,
    DspFailedToInitialize,
    PresetFailedToLoad,
    Running
};

struct AudioEngineStatus
{
    AudioEngineState state = AudioEngineState::BlackHoleNotInstalled;
    juce::String detail;

    juce::String userMessage() const;
    bool isHealthy() const { return state == AudioEngineState::Running; }
};
