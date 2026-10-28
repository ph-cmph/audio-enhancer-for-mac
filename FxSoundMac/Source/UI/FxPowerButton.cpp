// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include "FxPowerButton.h"
#include "FxTheme.h"

FxPowerButton::FxPowerButton() : juce::Button("power")
{
    setClickingTogglesState(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void FxPowerButton::setPowerState(bool on, juce::NotificationType n)
{
    setToggleState(on, n);
}

void FxPowerButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    const bool on = getToggleState();
    const auto accent = juce::Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f);
    const auto neutral = juce::Colour(FXCOLOR(DefaultText)).withAlpha(1.0f);
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour((on ? accent : neutral).withAlpha(on ? (down ? 0.24f : highlighted ? 0.2f : 0.12f) : (highlighted ? 0.1f : 0.055f)));
    g.fillEllipse(bounds);
    g.setColour((on ? accent : neutral).withAlpha(on ? 0.3f : 0.13f));
    g.drawEllipse(bounds.reduced(0.5f), 1.0f);
    const auto centre = bounds.getCentre();
    const float radius = bounds.getWidth() * 0.245f;
    juce::Path symbol;
    symbol.addCentredArc(centre.x, centre.y + 1.0f, radius, radius, 0, 0.65f,
                        juce::MathConstants<float>::twoPi - 0.65f, true);
    symbol.startNewSubPath(centre.x, centre.y - radius - 1.0f);
    symbol.lineTo(centre.x, centre.y - 1.0f);
    g.setColour(on ? accent : neutral.withAlpha(0.55f));
    g.strokePath(symbol, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (hasKeyboardFocus(true)) {
        g.setColour(accent.withAlpha(0.4f)); g.drawEllipse(bounds.expanded(1.0f), 1.2f);
    }
}
