// Independent macOS port modifications, 2026-10-29.
// AGPL-3.0-or-later. See LICENSE.
#include <juce_core/juce_core.h>
#include "../Source/UI/FxPalette.h"
#include <cmath>

namespace {
double luminance(std::uint32_t argb)
{
    const auto linear = [](unsigned channel) {
        const double value = channel / 255.0;
        return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * linear((argb >> 16) & 255) + 0.7152 * linear((argb >> 8) & 255)
         + 0.0722 * linear(argb & 255);
}
struct ThemePaletteTest : juce::UnitTest
{
    ThemePaletteTest() : juce::UnitTest("Appearance palette") {}
    void runTest() override
    {
        for (bool dark : {false, true})
        {
            beginTest(dark ? "Dark surfaces are opaque with readable text" : "Light surfaces are opaque with readable text");
            for (int i = 0; i < NumColors; ++i)
                expectEquals(static_cast<int>(FxPalette::color(static_cast<FxColor>(i), dark) >> 24), 255);
            for (auto surface : {WindowBackground, WidgetBackground, ControlBackground, ComboBoxBackground})
            {
                const auto text = luminance(FxPalette::color(DefaultText, dark));
                const auto background = luminance(FxPalette::color(surface, dark));
                expect((juce::jmax(text, background) + 0.05) / (juce::jmin(text, background) + 0.05) >= 7.0);
            }
        }
        beginTest("Appearance transitions replace the complete background");
        expect(FxPalette::color(WindowBackground, true) == std::uint32_t(0xff1c1c1e));
        expect(FxPalette::color(WindowBackground, false) == std::uint32_t(0xfff5f5f7));
    }
} themePaletteTest;
}
