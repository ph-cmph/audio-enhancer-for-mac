// Independent macOS port modifications, 2026-10-29.
// AGPL-3.0-or-later. See LICENSE.
#pragma once
#include "FxTheme.h"
#include <cmath>

namespace ValueReadout
{
inline juce::String text(float value, bool decibels)
{
    if (std::abs(value) < 0.05f) value = 0;
    return (decibels && value > 0 ? "+" : "") + juce::String(value, 1) + (decibels ? " dB" : "");
}
inline void draw(juce::Graphics& g, juce::Rectangle<int> bounds, float current, float original,
                 bool decibels, bool enabled, bool stacked)
{
    const auto primary = juce::Colour(FXCOLOR(DefaultText));
    const bool changed = std::abs(current - original) > 0.001f;
    const auto reference = "Preset " + text(original, decibels);
    if (stacked)
    {
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.setColour(changed ? juce::Colour(FXCOLOR(SliderTrack)).withAlpha(enabled ? 1.0f : 0.65f)
                            : primary.withAlpha(enabled ? 0.86f : 0.55f));
        g.drawText(text(current, decibels), bounds.removeFromTop(16), juce::Justification::centred);
        if (changed) {
            g.setFont(juce::Font(9.0f)); g.setColour(primary.withAlpha(0.56f));
            g.drawText(reference, bounds, juce::Justification::centred);
        }
    }
    else
    {
        const auto number = bounds.removeFromRight(30);
        g.setFont(juce::Font(11.5f, juce::Font::bold));
        g.setColour(changed ? juce::Colour(FXCOLOR(SliderTrack)).withAlpha(enabled ? 1.0f : 0.65f)
                            : primary.withAlpha(enabled ? 0.86f : 0.55f));
        g.drawText(text(current, false), number, juce::Justification::centredRight);
        if (changed) {
            g.setFont(juce::Font(9.0f)); g.setColour(primary.withAlpha(0.56f));
            g.drawText(reference, bounds.reduced(3, 0), juce::Justification::centredRight);
        }
    }
}
}
