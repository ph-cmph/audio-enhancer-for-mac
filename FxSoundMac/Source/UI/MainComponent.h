#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "FxPowerButton.h"
#include "FxEffects.h"
#include "FxEqualizer.h"
#include "../Support/FxController.h"
#include "../Support/PresetLibrary.h"
#include "../Support/PresetSession.h"
#include "../Audio/MacAudioEngine.h"
#include "../DSP/LegacyDspAdapter.h"

class MainComponent : public juce::Component, private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;

private:
    void refreshOutputDevices();
    void timerCallback() override;
    void saveSession();
    void restoreSession();
    juce::String lastSavedSession;
    juce::String preferredOutput;
    juce::StringArray knownOutputs;
    void refreshPresets();
    void setProcessingEnabled(bool on);
    void syncProcessingState();
    void handleEngineStatus(const AudioEngineStatus&);
    void reportError(const AudioEngineStatus&);
    juce::String pendingError;
    bool errorScheduled = false;
    void layoutTopRow();          // size preset/output combos to their content + reflow

    void requestPresetSwitch(const juce::File&);
    bool loadSelectedPreset(const juce::File&);
    void updatePresetDisplay();
    bool presetDialogOpen = false;
    void doSavePreset(std::function<void()> afterSave = {});          // prompt for a name, save current state as a custom preset
    void doImportPreset();        // pick a .fac off disk, copy into the user preset folder
    void doDeletePreset();        // delete the current preset (custom only)
    void showSettingsMenu();      // gear button → close-behavior preference

    static juce::File bundledPresetDir();
    static juce::File userPresetDir();   // ~/Library/Application Support/FxSound/Presets

    // FxController must be first — sub-components hold a reference to it
    FxController   controller;
    PresetSession presetSession { controller };
    MacAudioEngine engine { controller };
    PresetLibrary  presetLibrary;

    std::unique_ptr<juce::Drawable> logo_;
    std::unique_ptr<juce::Drawable> settingsIcon_;

    int currentPresetId_ = 0;                       // ComboBox id of the active preset
    std::unique_ptr<juce::FileChooser> fileChooser_; // kept alive during async import

    FxPowerButton        powerButton;
    juce::DrawableButton settingsButton_ { "settings", juce::DrawableButton::ImageFitted };
    juce::ComboBox   presetBox;
    juce::ComboBox   outputBox;
    juce::Label presetLabel { {}, "Preset" }, outputLabel { {}, "Output" };

    juce::Label  volumeLabel;
    juce::Slider volumeSlider;

    // FxEffects / FxEqualizer declared after controller so brace-init works
    FxEffects   effectsPanel  { controller };
    FxEqualizer equalizerPanel { controller, engine.activityTap() };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
