// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#pragma once
#include <juce_core/juce_core.h>

// Tiny persisted app preferences (survives relaunch).
// Stored as plain files under ~/Library/Application Support/FxSound Community/.
namespace AppSettings
{
    inline juce::File settingsDir()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("Application Support/FxSound Community");
    }

    // true  → close button quits the whole app (default)
    // false → close button hides the window; app keeps running (reopen via Dock icon)
    inline bool closeQuitsApp()
    {
        const auto f = settingsDir().getChildFile("close_quits.setting");
        if (f.existsAsFile())
            return f.loadFileAsString().trim() != "0";
        return true; // default
    }

    inline void setCloseQuitsApp(bool quits)
    {
        const auto f = settingsDir().getChildFile("close_quits.setting");
        f.getParentDirectory().createDirectory();
        f.replaceWithText(quits ? "1" : "0");
    }
    inline bool showVisualiser()
    {
        return settingsDir().getChildFile("visualiser.setting").loadFileAsString().trim() != "0";
    }
    inline void setShowVisualiser(bool show)
    {
        const auto f = settingsDir().getChildFile("visualiser.setting");
        f.getParentDirectory().createDirectory();
        f.replaceWithText(show ? "1" : "0");
    }
}
