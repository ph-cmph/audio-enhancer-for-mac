// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "MainWindow.h"
#include "FxTheme.h"

class FxSoundMacApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Audio Enhancer for Mac"; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String&) override
    {
        // Theme must be set before any Component is created.
        theme_ = std::make_unique<FxTheme>();
        juce::LookAndFeel::setDefaultLookAndFeel(theme_.get());

        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
        theme_ = nullptr;
    }

    void systemRequestedQuit() override { quit(); }

private:
    std::unique_ptr<FxTheme>     theme_;
    std::unique_ptr<MainWindow>  mainWindow;
};
