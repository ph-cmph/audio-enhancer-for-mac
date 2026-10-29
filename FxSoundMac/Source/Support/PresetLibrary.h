#pragma once
#include <juce_core/juce_core.h>

class PresetLibrary
{
public:
    // Scans the bundled (read-only) and user (custom, writable) preset folders.
    void scan(const juce::File& bundledDir, const juce::File& userDir);

    int getNumPresets() const { return presets.size(); }
    juce::String getPresetName(int index) const;
    juce::File getPresetFile(int index) const;
    bool isCustom(int index) const;                 // true → user preset (deletable)
    int  indexOfFile(const juce::File& file) const; // -1 if not present

    juce::File uniqueCustomFile(const juce::File& directory, const juce::String& requestedName) const;

private:
    struct Entry { juce::File file; bool custom; };
    juce::Array<Entry> presets; // sorted alphabetically by name, case-insensitive
};
