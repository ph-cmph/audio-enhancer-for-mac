// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include "SetupStatusPanel.h"

SetupStatusPanel::SetupStatusPanel()
{
    statusLabel.setJustificationType(juce::Justification::topLeft);
    statusLabel.setMinimumHorizontalScale(1.0f);
    addAndMakeVisible(statusLabel);
}

void SetupStatusPanel::setStatus(const AudioEngineStatus& status)
{
    statusLabel.setText(status.userMessage(), juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId,
                          status.isHealthy() ? juce::Colour(0xff8dc5ed)
                          : status.state == AudioEngineState::Stopped || status.state == AudioEngineState::BlackHoleNotInstalled
                            ? juce::Colour(0xffbcc8d6) : juce::Colour(0xffefbe81));
}

void SetupStatusPanel::resized()
{
    statusLabel.setBounds(getLocalBounds().reduced(8));
}
