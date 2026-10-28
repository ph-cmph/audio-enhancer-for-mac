// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#pragma once
#include <juce_core/juce_core.h>
#include "AudioEngineStatus.h"

namespace DeviceValidation
{
    // Returns the first device name containing "BlackHole" (case-insensitive), or empty.
    juce::String findBlackHoleDevice(const juce::StringArray& deviceNames);
    AudioEngineStatus validateRoute(const juce::String& capture, const juce::String& output, const juce::StringArray& availableOutputs);
    bool sampleRatesMatch(double a, double b, double tolerance = 0.5);
    bool isSupportedChannelLayout(int numChannels, int required = 2);
}
