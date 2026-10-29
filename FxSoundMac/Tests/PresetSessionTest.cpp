// Independent macOS port modifications, 2026-10-29. AGPL-3.0-or-later.
#include <juce_core/juce_core.h>
#include "PresetSession.h"

struct PresetSessionTest : juce::UnitTest
{
    PresetSessionTest() : juce::UnitTest("Preset editing and safe copies") {}
    void runTest() override
    {
        auto root = juce::File::getCurrentWorkingDirectory().getNonexistentChildFile("fx-preset-session", "", false);
        beginTest("Test directory is writable");
        expect(root.createDirectory().wasOk());
        const auto factory = juce::File(FXMAC_TEST_PRESET_DIR);
        const auto original = factory.getChildFile("General.fac");
        const auto originalText = original.loadFileAsString();
        FxController controller;
        PresetSession session(controller);
        PresetLibrary library;
        library.scan(factory, root);
        beginTest("A loaded preset is clean; effects and EQ independently mark it edited");
        expect(controller.prepare(48000, 512));
        expect(session.load(original));
        expect(!session.edited());
        const auto baseline = session.original();
        controller.setEffect(DfxDsp::Fidelity, 0.2f);
        expect(session.edited());
        baseline.apply(controller); expect(!session.edited());
        controller.setEqBand(2, 3.0f); expect(session.edited());
        baseline.apply(controller); expect(!session.edited());
        beginTest("Global volume and processing power are separate from preset edits");
        controller.setOutputGainDb(-6.0f); controller.setPower(true);
        expect(!session.edited());
        beginTest("A failed switch retains edits and the original reference");
        controller.setEqBand(2, 4.0f);
        const auto working = PresetValues::capture(controller);
        expect(!session.load(root.getChildFile("missing.fac")));
        expect(session.file() == original); expect(session.original().matches(baseline));
        expect(working.matches(PresetValues::capture(controller))); expect(session.edited());
        beginTest("Recovered working values remain dirty against the selected file");
        expect(session.load(original)); working.apply(controller); expect(session.edited());
        beginTest("Save a copy never overwrites factory presets or existing custom files");
        const auto existing = root.getChildFile("General 2.fac");
        existing.replaceWithText("An existing custom preset must survive");
        const auto saved = session.saveCopy(library, root, "general");
        expect(saved.existsAsFile()); expect(saved != existing); expect(saved != original);
        expectEquals(existing.loadFileAsString(), juce::String("An existing custom preset must survive"));
        expectEquals(original.loadFileAsString(), originalText);
        expect(!session.edited());
        expect(session.file() == saved);
        expect(session.load(saved));
        expectWithinAbsoluteError(controller.getEqBand(2), 4.0f, 0.01f);
        expectWithinAbsoluteError(controller.getEffect(DfxDsp::Fidelity), working.effects[0], 0.01f);
        beginTest("Saving again uses a new name even before the library is rescanned");
        const auto savedContents = saved.loadFileAsString();
        controller.setEqBand(2, 5.0f);
        const auto savedAgain = session.saveCopy(library, root, "general");
        expect(savedAgain.existsAsFile()); expect(savedAgain != saved);
        expectEquals(saved.loadFileAsString(), savedContents);
        beginTest("Save failures leave edited work available and keep the selected preset");
        controller.setEqBand(2, 6.0f);
        expect(session.saveCopy(library, root, "...") == juce::File{});
        const auto notDirectory = root.getChildFile("not-a-folder"); notDirectory.replaceWithText("file");
        expect(session.saveCopy(library, notDirectory, "copy") == juce::File{});
        expect(session.file() == savedAgain); expect(session.edited());
        expectWithinAbsoluteError(controller.getEqBand(2), 6.0f, 0.01f);
        root.deleteRecursively();
    }
} presetSessionTest;
