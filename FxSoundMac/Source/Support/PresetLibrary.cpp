#include "PresetLibrary.h"

void PresetLibrary::scan(const juce::File& bundledDir, const juce::File& userDir)
{
    presets.clear();

    auto addFrom = [this](const juce::File& dir, bool custom)
    {
        if (! dir.isDirectory())
            return;
        for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.fac"))
            presets.add({ f, custom });
    };

    addFrom(bundledDir, false);
    addFrom(userDir, true);

    // Sort by file name, case-insensitive, for stable UI ordering.
    std::sort(presets.begin(), presets.end(), [](const Entry& a, const Entry& b)
    {
        return a.file.getFileNameWithoutExtension().compareIgnoreCase(
               b.file.getFileNameWithoutExtension()) < 0;
    });
}

juce::String PresetLibrary::getPresetName(int index) const
{
    if (juce::isPositiveAndBelow(index, presets.size()))
        return presets[index].file.getFileNameWithoutExtension();
    return {};
}

juce::File PresetLibrary::getPresetFile(int index) const
{
    if (juce::isPositiveAndBelow(index, presets.size()))
        return presets[index].file;
    return {};
}

bool PresetLibrary::isCustom(int index) const
{
    if (juce::isPositiveAndBelow(index, presets.size()))
        return presets[index].custom;
    return false;
}

int PresetLibrary::indexOfFile(const juce::File& file) const
{
    for (int i = 0; i < presets.size(); ++i)
        if (presets[i].file == file)
            return i;
    return -1;
}

juce::File PresetLibrary::uniqueCustomFile(const juce::File& directory, const juce::String& requestedName) const
{
    auto base = juce::File::createLegalFileName(requestedName.trim()).trim().trimCharactersAtStart(".");
    if (base.endsWithIgnoreCase(".fac")) base = base.dropLastCharacters(4).trim();
    if (base.isEmpty()) return {};
    auto nameUsed = [&](const juce::String& name) {
        for (const auto& preset : presets)
            if (preset.file.getFileNameWithoutExtension().equalsIgnoreCase(name)) return true;
        for (const auto& file : directory.findChildFiles(juce::File::findFiles, false, "*.fac"))
            if (file.getFileNameWithoutExtension().equalsIgnoreCase(name)) return true;
        return directory.getChildFile(name + ".fac").exists();
    };
    auto candidate = base;
    for (int suffix = 2; nameUsed(candidate); ++suffix) candidate = base + " " + juce::String(suffix);
    return directory.getChildFile(candidate + ".fac");
}
