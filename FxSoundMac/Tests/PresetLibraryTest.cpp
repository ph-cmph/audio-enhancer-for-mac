// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include <juce_core/juce_core.h>
#include "PresetLibrary.h"

struct PresetLibraryTest : juce::UnitTest
{
    PresetLibraryTest() : juce::UnitTest("PresetLibrary") {}
    void runTest() override
    {
        auto tmp = juce::File::getCurrentWorkingDirectory().getNonexistentChildFile("preset-test", "", false);
        if (tmp.createDirectory().failed()) { beginTest("test directory is writable"); expect(false); return; }
        tmp.getChildFile("Rock.fac").create();
        tmp.getChildFile("Jazz.fac").create();
        tmp.getChildFile("notes.txt").create();

        beginTest("discovers only .fac files");
        PresetLibrary lib;
        lib.scan(tmp, juce::File{});
        expectEquals(lib.getNumPresets(), 2);

        beginTest("preset names are sorted and stripped of extension");
        expectEquals(lib.getPresetName(0), juce::String("Jazz"));
        expectEquals(lib.getPresetName(1), juce::String("Rock"));

        beginTest("file lookup by index returns the .fac file");
        expect(lib.getPresetFile(1).getFileName() == "Rock.fac");

        beginTest("empty directory yields zero presets");
        auto empty = tmp.getChildFile("empty");
        empty.createDirectory();
        PresetLibrary lib2;
        lib2.scan(empty, juce::File{});
        expectEquals(lib2.getNumPresets(), 0);

        tmp.deleteRecursively();
    }
};
static PresetLibraryTest presetLibraryTest;
