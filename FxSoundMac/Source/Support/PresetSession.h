// Independent macOS port modifications, 2026-10-29.
// AGPL-3.0-or-later; retain upstream notices. See LICENSE.
#pragma once
#include "FxController.h"
#include "PresetLibrary.h"
#include <array>
#include <cmath>
#include <vector>

struct PresetValues
{
    std::array<float, static_cast<size_t>(DfxDsp::NumEffects)> effects {};
    std::vector<float> eq;

    static PresetValues capture(const FxController& controller)
    {
        PresetValues values;
        for (size_t i = 0; i < values.effects.size(); ++i)
            values.effects[i] = controller.getEffect(static_cast<DfxDsp::Effect>(i));
        for (int i = 0; i < controller.getNumEqBands(); ++i) values.eq.push_back(controller.getEqBand(i));
        return values;
    }
    void apply(FxController& controller) const
    {
        for (size_t i = 0; i < effects.size(); ++i)
            controller.setEffect(static_cast<DfxDsp::Effect>(i), effects[i]);
        for (int i = 0; i < juce::jmin(static_cast<int>(eq.size()), controller.getNumEqBands()); ++i)
            controller.setEqBand(i, eq[static_cast<size_t>(i)]);
    }
    bool matches(const PresetValues& other) const
    {
        if (eq.size() != other.eq.size()) return false;
        for (size_t i = 0; i < effects.size(); ++i)
            if (std::abs(effects[i] - other.effects[i]) > 0.0001f) return false;
        for (size_t i = 0; i < eq.size(); ++i)
            if (std::abs(eq[i] - other.eq[i]) > 0.001f) return false;
        return true;
    }
};

// The reference is captured from the selected file, never from restored edits.
// File writes are explicit copies in the user's directory, never preset updates.
class PresetSession
{
public:
    explicit PresetSession(FxController& processor) : controller(processor) {}
    bool load(const juce::File& file)
    {
        const auto working = PresetValues::capture(controller);
        if (!controller.loadPreset(file)) {
            if (activeFile.existsAsFile()) controller.loadPreset(activeFile);
            working.apply(controller);
            return false;
        }
        activeFile = file;
        reference = PresetValues::capture(controller);
        hasReference = true;
        return true;
    }
    bool edited() const { return hasReference && !reference.matches(PresetValues::capture(controller)); }
    const PresetValues& original() const { return reference; }
    const juce::File& file() const { return activeFile; }
    juce::File saveCopy(const PresetLibrary& library, const juce::File& directory, const juce::String& name)
    {
        if (directory.createDirectory().failed()) return {};
        const auto destination = library.uniqueCustomFile(directory, name);
        if (destination == juce::File{} || destination.exists() || destination == activeFile) return {};
        if (!controller.savePreset(directory, destination.getFileNameWithoutExtension())) return {};
        if (!destination.existsAsFile() || destination.getSize() == 0) return {};
        activeFile = destination;
        reference = PresetValues::capture(controller);
        hasReference = true;
        return destination;
    }
private:
    FxController& controller;
    PresetValues reference;
    juce::File activeFile;
    bool hasReference = false;
};
