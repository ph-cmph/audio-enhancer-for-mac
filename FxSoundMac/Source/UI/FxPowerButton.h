// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class FxPowerButton : public juce::Button
{
public:
    FxPowerButton();
    ~FxPowerButton() override = default;

    void setPowerState(bool on, juce::NotificationType n = juce::dontSendNotification);
    bool getPowerState() const { return getToggleState(); }

private:
    void paintButton(juce::Graphics& g, bool, bool) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPowerButton)
};
