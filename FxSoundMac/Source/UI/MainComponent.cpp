// Independent macOS port modifications, 2026-10-28.
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

// Layout constants matching Windows FxProView proportions
constexpr int kMargin   = 24;
constexpr int kHeaderH  = 76;
constexpr int kComboH   = 60;
constexpr int kGap      = 16;
constexpr int kMainH    = 242;
constexpr int kBottomH  = 20;
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

        // Real preset: combo id maps to preset index (id - 1).
        const int idx = id - 1;
        if (idx >= 0)
        {
            currentPresetId_ = id;
            const bool ok = controller.loadPreset(presetLibrary.getPresetFile(idx));
            if (ok)
            {
                syncProcessingState();
                effectsPanel.update();
                equalizerPanel.update();
            }
            else
            {
                reportError({ AudioEngineState::PresetFailedToLoad,
                                        presetLibrary.getPresetName(idx) });
            }
        }
        layoutTopRow(); // selected name changed → resize combo to fit it
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

    // Volume control — "Volume" label above the themed slider, with the value shown
    // in a dB read-out box right next to the slider (themed dark box, no harsh border).
    volumeLabel.setText("Volume", juce::dontSendNotification);
    volumeLabel.setJustificationType(juce::Justification::centredLeft);
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
        controller.loadPreset(presetLibrary.getPresetFile(currentPresetId_ - 1));
        presetBox.setSelectedId(currentPresetId_, juce::dontSendNotification);
    }
    restoreSession();
    refreshOutputDevices();

    // Prime panels from current DSP state
    syncProcessingState();
    volumeSlider.setValue(controller.getOutputGainDb(), juce::dontSendNotification);
    effectsPanel.update();
    equalizerPanel.update();
    equalizerPanel.showValues(true);
    equalizerPanel.setVisualiserPreferences(AppSettings::showVisualiser(), systemPrefersReducedMotion());
    effectsPanel.showValues(true);

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
    presetBox.addItem("Save current as...", kSavePresetId);
    presetBox.addItem("Import...",          kImportPresetId);
    presetBox.addItem("Delete current",     kDeletePresetId);

    if (presetLibrary.getNumPresets() > 0)
    {
        int id = currentPresetId_;
        if (id <= 0 || id > presetLibrary.getNumPresets())
            id = 1;
        currentPresetId_ = id;
        presetBox.setSelectedId(id, juce::dontSendNotification);
    }
}

void MainComponent::doSavePreset()
{
    auto* aw = new juce::AlertWindow("Save Preset",
                                     "Name your preset:",
                                     juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor("name", {}, {});
    aw->addButton("Save",   1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    aw->enterModalState(true, juce::ModalCallbackFunction::create([safe = juce::Component::SafePointer<MainComponent>(this), aw](int result)
    {
        if (safe == nullptr) return;
        auto* self = safe.getComponent();
        if (result == 0)
            return;

        const juce::String name = aw->getTextEditorContents("name").trim();
        if (name.isEmpty())
            return;

        const juce::String safeName = juce::File::createLegalFileName(name);
        const juce::File dest = self->userPresetDir().getChildFile(safeName + ".fac");

        self->userPresetDir().createDirectory();
        if (self->controller.savePreset(self->userPresetDir(), dest.getFileNameWithoutExtension()))
        {
            self->refreshPresets();
            const int idx = self->presetLibrary.indexOfFile(dest);
            if (idx >= 0)
            {
                self->currentPresetId_ = idx + 1;
                self->presetBox.setSelectedId(self->currentPresetId_, juce::dontSendNotification);
                self->layoutTopRow();
            }
        }
        else
        {
            self->reportError({ AudioEngineState::PresetFailedToLoad, name });
        }
    }), true);
}

void MainComponent::doImportPreset()
{
    fileChooser_ = std::make_unique<juce::FileChooser>(
        "Import preset (.fac)",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.fac");

    fileChooser_->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const juce::File src = fc.getResult();
            if (! src.existsAsFile())
                return;

            userPresetDir().createDirectory();
            const juce::File dest = userPresetDir().getChildFile(src.getFileName());
            if (src.copyFileTo(dest))
            {
                refreshPresets();
                const int idx = presetLibrary.indexOfFile(dest);
                if (idx >= 0)
                    presetBox.setSelectedId(idx + 1, juce::sendNotification); // load it
            }
            else
            {
                reportError({ AudioEngineState::PresetFailedToLoad,
                                        src.getFileNameWithoutExtension() });
            }
        });
}

void MainComponent::doDeletePreset()
{
    const int idx = currentPresetId_ - 1;
    if (! juce::isPositiveAndBelow(idx, presetLibrary.getNumPresets()))
        return;

    if (! presetLibrary.isCustom(idx))
    {
        juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
            "Can't delete", "Factory presets can't be deleted.");
        return;
    }

    const juce::File file = presetLibrary.getPresetFile(idx);
    const juce::String name = presetLibrary.getPresetName(idx);

    juce::NativeMessageBox::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon,
        "Delete preset", "Delete \"" + name + "\"? The preset will be moved to the Trash.", nullptr,
        juce::ModalCallbackFunction::create([this, file](int r)
        {
            if (r != 1)
                return;
            if (!file.moveToTrash()) { reportError({AudioEngineState::PresetFailedToLoad, "Could not move this preset to the Trash."}); return; }
            currentPresetId_ = 1;
            refreshPresets();
            presetBox.setSelectedId(currentPresetId_, juce::dontSendNotification);
            layoutTopRow();
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
    data->setProperty("preset", presetBox.getText());
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
    if (auto* effects = data["effects"].getArray())
        for (int i = 0; i < juce::jmin(effects->size(), (int)DfxDsp::NumEffects); ++i)
            controller.setEffect(static_cast<DfxDsp::Effect>(i), (float)(*effects)[i]);
    if (auto* eq = data["eq"].getArray())
        for (int i = 0; i < juce::jmin(eq->size(), controller.getNumEqBands()); ++i) controller.setEqBand(i, (float)(*eq)[i]);
    for (int i = 0; i < presetLibrary.getNumPresets(); ++i)
        if (presetLibrary.getPresetName(i) == data["preset"].toString()) {
            currentPresetId_ = i + 1;
            presetBox.setSelectedId(currentPresetId_, juce::dontSendNotification);
        }
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
    g.fillAll(juce::Colour(FXCOLOR(WindowBackground)));

    g.setColour(juce::Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f));
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText("Audio Enhancer", 24, 12, 150, 29, juce::Justification::centredLeft);
    g.setColour(juce::Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
    g.setFont(juce::Font(12.0f));
    g.setColour(juce::Colour(FXCOLOR(DefaultText)).withAlpha(0.6f));
    g.drawText("for Mac", 25, 43, 130, 17, juce::Justification::centredLeft);
}

void MainComponent::lookAndFeelChanged()
{
    volumeSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(FXCOLOR(ControlBackground)).withAlpha(1.0f));
    volumeSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
    const auto secondary = juce::Colour(FXCOLOR(DefaultText)).withAlpha(0.6f);
    presetLabel.setColour(juce::Label::textColourId, secondary);
    outputLabel.setColour(juce::Label::textColourId, secondary);
    repaint();
}

void MainComponent::resized()
{
    // Header: power button (left) + volume control right beside it, styled like the
    // effect sliders ("Volume" label above, value next to the thumb, no dB box).
    powerButton.setBounds(180, 17, 42, 42);
    settingsButton_.setBounds(getWidth() - kMargin - 24, 24, 24, 24);

    const int volBlockW = 248;
    const int volX = powerButton.getRight() + 24;
    volumeLabel.setBounds(volX, 14, 80, 18);
    volumeSlider.setBounds(volX, 35, volBlockW, 24);

    // Combos row — widths fit their content (see layoutTopRow)
    layoutTopRow();

    // Main panels — matching Windows FxProView geometry
    const int mainY = kHeaderH + kComboH + kGap;
    effectsPanel.setBounds(kMargin, mainY, 196, 242);
    equalizerPanel.setBounds(kMargin + 196 + 16, mainY, getWidth() - kMargin * 2 - 196 - 16, 242);

}

// Size the preset + output combos to fit their currently-selected text (with a
// sensible min/max), then lay them left-to-right. Avoids the fixed oversized boxes.
void MainComponent::layoutTopRow()
{
    const int comboY = kHeaderH + 20;
    const int comboH = 32;

    auto contentWidth = [](juce::ComboBox& box, int minW, int maxW)
    {
        juce::Font f = box.getLookAndFeel().getComboBoxFont(box);
        juce::String t = box.getText();
        if (t.isEmpty()) t = box.getTextWhenNothingSelected();
        const int w = f.getStringWidth(t) + 10 /*left pad*/ + 42 /*arrow + right pad*/;
        return juce::jlimit(minW, maxW, w);
    };

    presetBox.setBounds(kMargin, comboY, contentWidth(presetBox, 196, 320), comboH);
    outputBox.setBounds(presetBox.getRight() + 12, comboY,
                        contentWidth(outputBox, 160, 460), comboH);
    presetLabel.setBounds(presetBox.getX(), kHeaderH, presetBox.getWidth(), 16);
    outputLabel.setBounds(outputBox.getX(), kHeaderH, outputBox.getWidth(), 16);
}
