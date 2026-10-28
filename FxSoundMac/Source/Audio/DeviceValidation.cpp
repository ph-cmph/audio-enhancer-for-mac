// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include "DeviceValidation.h"
#include "AudioEngineStatus.h"
#include <cmath>

namespace DeviceValidation
{
    AudioEngineStatus validateRoute(const juce::String& capture, const juce::String& output, const juce::StringArray& availableOutputs)
    {
        if (!capture.trim().equalsIgnoreCase("BlackHole 2ch")) return {AudioEngineState::BlackHoleNotInstalled, {}};
        if (output.trim().isEmpty()) return {AudioEngineState::NoOutputSelected, {}};
        if (output.containsIgnoreCase("BlackHole")) return {AudioEngineState::InvalidOutput, {}};
        if (!availableOutputs.contains(output)) return {AudioEngineState::DeviceUnavailable, output};
        return {AudioEngineState::Stopped, {}};
    }

    juce::String findBlackHoleDevice(const juce::StringArray& deviceNames)
    {
        for (const auto& name : deviceNames)
            if (name.trim().equalsIgnoreCase("BlackHole 2ch"))
                return name;
        return {};
    }

    bool sampleRatesMatch(double a, double b, double tolerance)
    {
        return std::isfinite(a) && std::isfinite(b) && a > 0 && b > 0 && tolerance >= 0 && std::abs(a - b) <= tolerance;
    }

    bool isSupportedChannelLayout(int numChannels, int required)
    {
        return numChannels == required;
    }
}

juce::String AudioEngineStatus::userMessage() const
{
    switch (state)
    {
        case AudioEngineState::Stopped:
            return "Ready. Choose your speakers or headphones, then start processing.";
        case AudioEngineState::InvalidOutput:
            return "Choose speakers or headphones as the output. Sending sound back into BlackHole creates a feedback loop.";
        case AudioEngineState::DeviceUnavailable:
            return "The audio device changed or disconnected. Normal audio has been restored; choose an available output and start again. " + detail;
        case AudioEngineState::RouteFailed:
            return "Could not route Mac audio. Processing was stopped and your output was kept unchanged.";
        case AudioEngineState::VolumeSetupFailed:
            return "Could not synchronise audio volume. Processing was stopped; check your output device and try again.";
        case AudioEngineState::RestoreFailed:
            return "Processing stopped, but your previous output could not be restored. Select your speakers in System Settings > Sound.";
        case AudioEngineState::BlackHoleNotInstalled:
            return "BlackHole 2ch is needed to process Mac audio. Install the official driver from Existential Audio, restart your Mac, then reopen FxSound.";
        case AudioEngineState::BlackHoleNotSelected:
            return "BlackHole is installed but not selected as the input. Route system output to BlackHole 2ch.";
        case AudioEngineState::NoOutputSelected:
            return "Select a physical output device for playback.";
        case AudioEngineState::SampleRateMismatch:
            return "Input and output sample rates do not match. Set both to the same rate in Audio MIDI Setup. " + detail;
        case AudioEngineState::UnsupportedChannelLayout:
            return "Unsupported channel layout. FxSoundMac requires a stereo (2ch) route. " + detail;
        case AudioEngineState::EngineFailedToStart:
            return "The audio engine failed to start. " + detail;
        case AudioEngineState::DspFailedToInitialize:
            return "The DSP failed to initialize. " + detail;
        case AudioEngineState::PresetFailedToLoad:
            return "The selected preset failed to load. " + detail;
        case AudioEngineState::Running:
            return "Processing Mac audio: " + detail + ". Stop to restore normal playback.";
    }
    return "Unknown state.";
}
