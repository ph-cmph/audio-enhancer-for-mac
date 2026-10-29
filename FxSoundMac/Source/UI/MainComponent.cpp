// Independent macOS port modifications, 2026-10-29.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include "MainComponent.h"
#include "FxTheme.h"
#include "../Audio/DeviceValidation.h"
#include "../Support/AppSettings.h"
#include <BinaryData.h>
#include "NativeMotion.h"

namespace {
// ComboBox ids for the preset-management actions (kept clear of real preset ids 1..N)
constexpr int kSavePresetId   = 9001;
constexpr int kImportPresetId = 9002;
constexpr int kDeletePresetId = 9003;

// Two quiet toolbar rows above the main processing panels.
constexpr int kMargin   = 24;
constexpr int kHeaderH  = 68;
constexpr int kComboH   = 60;
constexpr int kGap      = 16;
constexpr int kMainH    = 260;
constexpr int kBottomH  = 20;

void stylePresetDialog(juce::AlertWindow& alert)
{
    // A single primary action; quieter alternatives keep the decision easy to scan.
    for (int i = 0; i < alert.getNumButtons(); ++i) {
        if (auto* button = alert.getButton(i)) {
            const auto background = juce::Colour(FxTheme::getColor(i == 0 ? FxColor::TextButtonBackground : FxColor::ComboBoxBackground));
            const auto text = i == 0 ? juce::Colours::white : juce::Colour(FXCOLOR(DefaultText));
            button->setColour(juce::TextButton::buttonColourId, background);
            button->setColour(juce::TextButton::textColourOffId, text);
        }
    }
}
} // namespace

juce::File MainComponent::bundledPresetDir()
{
    return juce::File::getSpecialLocation(juce::File::currentApplicationFile)
               .getChildFile("Contents/Resources/Presets");
}

juce::File MainComponent::userPresetDir()
{
    // Writable, persists across launches: ~/Library/Application Support/FxSound Community/Presets
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
               .getChildFile("Application Support/FxSound Community/Presets");
}

MainComponent::MainComponent()
{
    setOpaque(true);
    controller.prepare(48000, 512);
    controller.setPower(false);

    // Logo SVG (drawn in paint)
    // Independent community build: use a clear text title instead of official branding.

    // Power button
    powerButton.setPowerState(false);
    powerButton.onClick = [this] { setProcessingEnabled(powerButton.getToggleState()); };
    addAndMakeVisible(powerButton);

    // Settings (gear) button — top-right; opens the close-behavior preference.
    settingsIcon_ = juce::Drawable::createFromImageData(BinaryData::settings_svg,
                                                        BinaryData::settings_svgSize);
    settingsButton_.setImages(settingsIcon_.get());
    settingsButton_.setColour(juce::DrawableButton::backgroundColourId,   juce::Colours::transparentBlack);
    settingsButton_.setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);
    settingsButton_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    settingsButton_.onClick = [this] { showSettingsMenu(); };
    addAndMakeVisible(settingsButton_);

    // Preset selector
    addAndMakeVisible(presetBox);
    presetBox.setTextWhenNothingSelected("Select Preset");
    presetBox.onChange = [this] {
        const int id = presetBox.getSelectedId();

        // Action items: run the action, restore the displayed selection, don't "load" them.
        if (id == kSavePresetId || id == kImportPresetId || id == kDeletePresetId)
        {
            presetBox.setSelectedId(currentPresetId_, juce::dontSendNotification);
            if      (id == kSavePresetId)   doSavePreset();
            else if (id == kImportPresetId) doImportPreset();
            else                            doDeletePreset();
            return;
        }

        const auto file = presetLibrary.getPresetFile(id - 1);
        presetBox.setSelectedId(currentPresetId_, juce::dontSendNotification);
        if (file != juce::File{}) requestPresetSwitch(file);
    };

    // Output selector
    addAndMakeVisible(outputBox);
    for (auto* label : {&presetLabel, &outputLabel}) {
        label->getProperties().set("useOwnFont", true);
        label->setFont(juce::Font(11.0f));
        label->setBorderSize(juce::BorderSize<int>(0));
        addAndMakeVisible(label);
    }
    outputBox.setTextWhenNothingSelected("Select Output");
    outputBox.onChange = [this] {
        preferredOutput = outputBox.getText();
        layoutTopRow();
        // Live re-route: if audio is already running, switch it to the new device.
        // The single power control follows the actual engine state after rerouting.
        if (engine.isRunning())
            engine.setOutputDevice(outputBox.getText());
    };

    // Volume shares the selectors row; all three labels use the same typography.
    volumeLabel.setText("Volume", juce::dontSendNotification);
    volumeLabel.setJustificationType(juce::Justification::centredLeft);
    volumeLabel.getProperties().set("useOwnFont", true);
    volumeLabel.setFont(juce::Font(11.0f));
    volumeLabel.setBorderSize(juce::BorderSize<int>(0));
    addAndMakeVisible(volumeLabel);

    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setRange(LegacyDspAdapter::minOutputGainDb, LegacyDspAdapter::maxOutputGainDb, 1.0);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 58, 24);
    volumeSlider.setTextValueSuffix(" dB");
    volumeSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(FXCOLOR(ControlBackground)).withAlpha(1.0f));
    volumeSlider.setColour(juce::Slider::textBoxTextColourId,        juce::Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
    volumeSlider.setColour(juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);
    volumeSlider.setValue(controller.getOutputGainDb(), juce::dontSendNotification);
    volumeSlider.onValueChange = [this] {
        controller.setOutputGainDb((float) volumeSlider.getValue());
    };
    addAndMakeVisible(volumeSlider);

    // Main panels
    addAndMakeVisible(effectsPanel);
    addAndMakeVisible(equalizerPanel);
    effectsPanel.onEdited = equalizerPanel.onEdited = [this] { updatePresetDisplay(); };

    // Engine notifications are delivered on the message thread. SafePointer also
    // protects shutdown and future asynchronous device notifications.
    engine.onStatusChanged = [safe = juce::Component::SafePointer<MainComponent>(this)](AudioEngineStatus next) {
        if (safe != nullptr) {
            safe->handleEngineStatus(next);
        }
    };

    // Populate dropdowns
    refreshPresets();
    for (int i = 0; i < presetLibrary.getNumPresets(); ++i)
        if (presetLibrary.getPresetName(i) == "General") currentPresetId_ = i + 1;
    if (currentPresetId_ > 0) {
        presetSession.load(presetLibrary.getPresetFile(currentPresetId_ - 1));
        presetBox.setSelectedId(currentPresetId_, juce::dontSendNotification);
    }
    restoreSession();
    refreshOutputDevices();

    // Prime panels from current DSP state
    syncProcessingState();
    volumeSlider.setValue(controller.getOutputGainDb(), juce::dontSendNotification);
    effectsPanel.update();
    equalizerPanel.update();
    updatePresetDisplay();
    equalizerPanel.setVisualiserPreferences(AppSettings::showVisualiser(), systemPrefersReducedMotion());


    // Reopening always starts grey/off. Only the power control starts routing.
    if (engine.getStatus().state == AudioEngineState::RestoreFailed) reportError(engine.getStatus());

    startTimer(2000);
    setSize(1040, kHeaderH + kComboH + kGap + kMainH + kBottomH);
}

MainComponent::~MainComponent()
{
    stopTimer();
    saveSession();
    engine.onStatusChanged = {};
    engine.stop();
}

void MainComponent::syncProcessingState()
{
    const bool on = engine.isRunning();
    controller.setPower(on);
    powerButton.setPowerState(on, juce::dontSendNotification);
    powerButton.setButtonText(on ? "Stop audio processing" : "Start audio processing");
    powerButton.setTooltip(on ? "Turn audio processing off" : "Turn audio processing on");
    effectsPanel.setEnabled(on);
    equalizerPanel.setControlsEnabled(on);
    equalizerPanel.setProcessingState(on);
}

void MainComponent::setProcessingEnabled(bool on)
{
    if (on) { if (!engine.isRunning()) engine.start(outputBox.getText()); }
    else engine.stop();
    syncProcessingState();
}

void MainComponent::handleEngineStatus(const AudioEngineStatus& status)
{
    syncProcessingState();
    if (status.state == AudioEngineState::Running) pendingError.clear();
    else if (status.state != AudioEngineState::Stopped) reportError(status);
}

void MainComponent::reportError(const AudioEngineStatus& status)
{
    pendingError = status.userMessage();
    if (errorScheduled) return;
    errorScheduled = true;
    // Start/stop can issue several synchronous status updates. Present only the
    // final actionable failure, after the audio route has finished cleaning up.
    juce::MessageManager::callAsync([safe = juce::Component::SafePointer<MainComponent>(this)] {
        if (safe == nullptr) return;
        safe->errorScheduled = false;
        const auto message = safe->pendingError;
        safe->pendingError.clear();
        if (message.isNotEmpty()) juce::NativeMessageBox::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon, "FxSound", message, safe.getComponent());
    });
}

void MainComponent::refreshPresets()
{
    userPresetDir().createDirectory();
    presetLibrary.scan(bundledPresetDir(), userPresetDir());

    presetBox.clear(juce::dontSendNotification);
    for (int i = 0; i < presetLibrary.getNumPresets(); ++i)
        presetBox.addItem(presetLibrary.getPresetName(i), i + 1);

    presetBox.addSeparator();
    presetBox.addItem("Save a Custom Copy...", kSavePresetId);
    presetBox.addItem("Import...",          kImportPresetId);
    presetBox.addItem("Delete current",     kDeletePresetId);

    const int active = presetLibrary.indexOfFile(presetSession.file());
    currentPresetId_ = active >= 0 ? active + 1 : (presetLibrary.getNumPresets() > 0 ? 1 : 0);
    presetBox.setSelectedId(currentPresetId_, juce::dontSendNotification);
}

void MainComponent::updatePresetDisplay()
{
    effectsPanel.setReference(presetSession.original());
    equalizerPanel.setReference(presetSession.original());
    presetLabel.setText(presetSession.edited() ? "Preset - Edited" : "Preset", juce::dontSendNotification);
}

bool MainComponent::loadSelectedPreset(const juce::File& file)
{
    if (!presetSession.load(file)) {
        reportError({AudioEngineState::PresetFailedToLoad, file.getFileNameWithoutExtension()});
        return false;
    }
    currentPresetId_ = presetLibrary.indexOfFile(file) + 1;
    presetBox.setSelectedId(currentPresetId_, juce::dontSendNotification);
    effectsPanel.update(); equalizerPanel.update();
    updatePresetDisplay();
    syncProcessingState();
    return true;
}

void MainComponent::requestPresetSwitch(const juce::File& file)
{
    if (presetDialogOpen || file == presetSession.file()) return;
    if (!presetSession.edited()) { loadSelectedPreset(file); return; }
    presetDialogOpen = true;
    auto* alert = new juce::AlertWindow("Save your changes?",
        "You edited \"" + presetSession.file().getFileNameWithoutExtension() + "\". Save a custom copy before switching presets?\nThe original preset stays unchanged.",
        juce::MessageBoxIconType::NoIcon);
    alert->addButton("Save a Copy...", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert->addButton("Discard", 2);
    alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    stylePresetDialog(*alert);
    alert->enterModalState(true, juce::ModalCallbackFunction::create(
        [safe = juce::Component::SafePointer<MainComponent>(this), file](int result) {
            if (safe == nullptr) return;
            safe->presetDialogOpen = false;
            if (result == 2) safe->loadSelectedPreset(file);
            else if (result == 1) safe->doSavePreset([safe, file] {
                if (safe != nullptr) safe->loadSelectedPreset(file);
            });
        }), true);
}

void MainComponent::doSavePreset(std::function<void()> afterSave)
{
    if (presetDialogOpen) return;
    presetDialogOpen = true;
    auto* alert = new juce::AlertWindow("Save a Custom Copy", "Give this version a name. Existing presets will not be replaced.", juce::MessageBoxIconType::NoIcon);
    alert->addTextEditor("name", presetSession.file().getFileNameWithoutExtension() + " Custom", {});
    alert->addButton("Save Copy", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    stylePresetDialog(*alert);
    alert->enterModalState(true, juce::ModalCallbackFunction::create(
        [safe = juce::Component::SafePointer<MainComponent>(this), alert, afterSave](int result) {
            if (safe == nullptr) return;
            safe->presetDialogOpen = false;
            if (result == 0) return;
            const auto name = alert->getTextEditorContents("name").trim();
            const auto saved = safe->presetSession.saveCopy(safe->presetLibrary, userPresetDir(), name);
            if (saved == juce::File{}) {
                juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                    "Couldn't save copy", "Choose a valid name and a writable preset folder. Your changes are still here.", safe.getComponent());
                return;
            }
            safe->refreshPresets(); safe->updatePresetDisplay();
            if (afterSave) afterSave();
        }), true);
}

void MainComponent::doImportPreset()
{
    if (presetDialogOpen) return;
    fileChooser_ = std::make_unique<juce::FileChooser>("Import preset (.fac)",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.fac");
    fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe = juce::Component::SafePointer<MainComponent>(this)](const juce::FileChooser& chooser) {
            if (safe == nullptr) return;
            const auto source = chooser.getResult();
            if (!source.existsAsFile()) return;
            const auto dest = safe->presetLibrary.uniqueCustomFile(userPresetDir(), source.getFileNameWithoutExtension());
            if (dest != juce::File{} && !dest.exists() && source.copyFileTo(dest)) {
                safe->refreshPresets(); safe->requestPresetSwitch(dest);
            } else safe->reportError({AudioEngineState::PresetFailedToLoad, source.getFileNameWithoutExtension()});
        });
}

void MainComponent::doDeletePreset()
{
    const int index = presetLibrary.indexOfFile(presetSession.file());
    if (index < 0 || presetDialogOpen) return;
    if (!presetLibrary.isCustom(index)) {
        juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
            "Can't delete", "Factory presets can't be deleted.", this);
        return;
    }
    // Keep edited work available; deleting the saved file is a separate explicit action.
    if (presetSession.edited()) {
        juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
            "Save your changes first", "Save a custom copy or switch presets and discard your edits before deleting this preset.", this);
        return;
    }
    const auto file = presetSession.file();
    juce::File replacement;
    for (int i = 0; i < presetLibrary.getNumPresets(); ++i)
        if (!presetLibrary.isCustom(i)) { replacement = presetLibrary.getPresetFile(i); break; }
    if (replacement == juce::File{}) return;
    presetDialogOpen = true;
    juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon,
        "Delete preset", "Move \"" + file.getFileNameWithoutExtension() + "\" to the Trash?", this,
        juce::ModalCallbackFunction::create([safe = juce::Component::SafePointer<MainComponent>(this), file, replacement](int result) {
            if (safe == nullptr) return;
            safe->presetDialogOpen = false;
            if (result != 1) return;
            if (!file.moveToTrash()) { safe->reportError({AudioEngineState::PresetFailedToLoad, "Could not move this preset to the Trash."}); return; }
            safe->refreshPresets(); safe->loadSelectedPreset(replacement);
        }));
}

void MainComponent::refreshOutputDevices()
{
    juce::StringArray outputs;
    for (const auto& name : engine.getOutputDeviceNames())
        if (!name.containsIgnoreCase("BlackHole")) outputs.add(name);
    if (outputs == knownOutputs && outputBox.getNumItems() > 0) return;
    knownOutputs = outputs;
    const auto selected = outputBox.getText().isNotEmpty() ? outputBox.getText() : preferredOutput;
    outputBox.clear(juce::dontSendNotification);
    int id = 1;
    for (const auto& name : outputs) outputBox.addItem(name, id++);
    if (selected.isNotEmpty()) outputBox.setText(selected, juce::dontSendNotification);
    else if (outputs.size() > 0) {
        outputBox.setSelectedId(1, juce::dontSendNotification);
        preferredOutput = outputBox.getText();
    }
    layoutTopRow();
}

void MainComponent::timerCallback()
{
    refreshOutputDevices(); saveSession();
    equalizerPanel.setVisualiserPreferences(AppSettings::showVisualiser(), systemPrefersReducedMotion());
}
void MainComponent::saveSession()
{
    juce::DynamicObject::Ptr data = new juce::DynamicObject();
    data->setProperty("output", preferredOutput);
    data->setProperty("gain", controller.getOutputGainDb());
    data->setProperty("preset", presetSession.file().getFileNameWithoutExtension());
    data->setProperty("presetFile", presetSession.file().getFullPathName());
    juce::Array<juce::var> effects, eq;
    for (int i = 0; i < DfxDsp::NumEffects; ++i) effects.add(controller.getEffect(static_cast<DfxDsp::Effect>(i)));
    for (int i = 0; i < controller.getNumEqBands(); ++i) eq.add(controller.getEqBand(i));
    data->setProperty("effects", effects);
    data->setProperty("eq", eq);
    const auto json = juce::JSON::toString(juce::var(data));
    if (json == lastSavedSession) return;
    const auto file = AppSettings::settingsDir().getChildFile("session.json");
    if (file.getParentDirectory().createDirectory().failed()) return;
    juce::TemporaryFile temporary(file);
    if (temporary.getFile().replaceWithText(json) && temporary.overwriteTargetFileWithTemporary()) lastSavedSession = json;
}
void MainComponent::restoreSession()
{
    const auto data = juce::JSON::parse(AppSettings::settingsDir().getChildFile("session.json"));
    if (!data.isObject()) return;
    preferredOutput = data["output"].toString();
    // Earlier releases saved an effects-bypass flag. It is deliberately ignored:
    // the illuminated button now means a running engine, never a remembered wish.
    if (data.hasProperty("gain")) controller.setOutputGainDb((float)data["gain"]);
    // Load the actual saved preset before applying recovered working edits.
    // This preserves its reference values across restarts and retains its frequencies.
    auto selected = data["presetFile"].toString();
    int index = selected.isNotEmpty() ? presetLibrary.indexOfFile(juce::File(selected)) : -1;
    if (index < 0)
        for (int i = 0; i < presetLibrary.getNumPresets(); ++i)
            if (presetLibrary.getPresetName(i) == data["preset"].toString()) { index = i; break; }
    if (index >= 0) loadSelectedPreset(presetLibrary.getPresetFile(index));
    if (auto* effects = data["effects"].getArray())
        for (int i = 0; i < juce::jmin(effects->size(), (int)DfxDsp::NumEffects); ++i)
            controller.setEffect(static_cast<DfxDsp::Effect>(i), (float)(*effects)[i]);
    if (auto* eq = data["eq"].getArray())
        for (int i = 0; i < juce::jmin(eq->size(), controller.getNumEqBands()); ++i)
            controller.setEqBand(i, (float)(*eq)[i]);
}

void MainComponent::showSettingsMenu()
{
    juce::PopupMenu m;
    const bool quits = AppSettings::closeQuitsApp();
    m.addSectionHeader("Audio Enhancer for Mac");
    m.addItem(3, "About & licence");
    m.addSeparator();
    m.addItem(4, "Show audio visualiser", true, AppSettings::showVisualiser());
    m.addSeparator();
    m.addSectionHeader("Close button behavior");
    m.addItem(1, "Quit FxSound",                 true, quits);
    m.addItem(2, "Keep running (hide window)",   true, !quits);

    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(settingsButton_),
        [safe = juce::Component::SafePointer<MainComponent>(this)](int result)
        {
            if (result == 3) juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                "Audio Enhancer for Mac", "An independent macOS build based on FxSound and community ports.\nNot an official FxSound release.\n\nLicensed under AGPL-3.0-or-later. Copyright FxSound LLC and respective contributors. You may redistribute and modify this application under that licence. There is no warranty. Full licence text is included in the app Resources folder; corresponding source and build scripts accompany every release. Source: https://github.com/ph-cmph/audio-enhancer-for-mac");
            if (result == 1)      AppSettings::setCloseQuitsApp(true);
            else if (result == 2) AppSettings::setCloseQuitsApp(false);
            else if (result == 4) {
                AppSettings::setShowVisualiser(!AppSettings::showVisualiser());
                if (safe != nullptr) safe->equalizerPanel.setVisualiserPreferences(AppSettings::showVisualiser(), systemPrefersReducedMotion());
            }
        });
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(FXCOLOR(WindowBackground))); // Opaque in both appearances.

    g.setColour(juce::Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f));
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText("Audio Enhancer", 24, 10, 150, 26, juce::Justification::centredLeft);
    g.setColour(juce::Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
    g.setFont(juce::Font(12.0f));
    g.setColour(juce::Colour(FXCOLOR(DefaultText)).withAlpha(0.6f));
    g.drawText("for Mac", 25, 39, 130, 16, juce::Justification::centredLeft);
}

void MainComponent::lookAndFeelChanged()
{
    volumeSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(FXCOLOR(ControlBackground)).withAlpha(1.0f));
    volumeSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
    const auto secondary = juce::Colour(FXCOLOR(DefaultText)).withAlpha(0.6f);
    presetLabel.setColour(juce::Label::textColourId, secondary);
    outputLabel.setColour(juce::Label::textColourId, secondary);
    volumeLabel.setColour(juce::Label::textColourId, secondary);
    repaint();
}

void MainComponent::resized()
{
    // Title on the left; the two global actions form a small right-aligned group.
    settingsButton_.setBounds(getWidth() - kMargin - 24, 21, 24, 24);
    powerButton.setBounds(settingsButton_.getX() - 16 - 42, 12, 42, 42);

    // Preset, playback device and volume share one stable, full-width row.
    layoutTopRow();

    // Keep the main panels roomy beneath the compact toolbar.
    const int mainY = kHeaderH + kComboH + kGap;
    effectsPanel.setBounds(kMargin, mainY, 244, kMainH);
    equalizerPanel.setBounds(kMargin + 244 + 16, mainY, getWidth() - kMargin * 2 - 244 - 16, kMainH);

}

// Stable widths keep controls from shifting when a preset or device changes.
// Generous gaps separate the three groups without leaving an unused right half.
void MainComponent::layoutTopRow()
{
    if (getWidth() == 0) return; // Selectors can change before the initial size is set.
    const int comboY = kHeaderH + 20;
    constexpr int comboH = 32;
    constexpr int groupGap = 24;
    constexpr int volumeWidth = 280;
    const int selectorsWidth = getWidth() - kMargin * 2 - groupGap * 2 - volumeWidth;
    const int presetWidth = selectorsWidth * 44 / 100;
    const int outputWidth = selectorsWidth - presetWidth;

    presetBox.setBounds(kMargin, comboY, presetWidth, comboH);
    outputBox.setBounds(presetBox.getRight() + groupGap, comboY, outputWidth, comboH);
    volumeSlider.setBounds(outputBox.getRight() + groupGap, comboY + 4, volumeWidth, 24);
    presetLabel.setBounds(presetBox.getX(), kHeaderH, presetBox.getWidth(), 16);
    outputLabel.setBounds(outputBox.getX(), kHeaderH, outputBox.getWidth(), 16);
    volumeLabel.setBounds(volumeSlider.getX(), kHeaderH, volumeSlider.getWidth(), 16);
}
