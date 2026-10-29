// Independent macOS port modifications, 2026-10-29.
// AGPL-3.0-or-later; retain upstream notices. See LICENSE.
#pragma once
#include <cstdint>

enum FxColor : int
{
    WindowBackground, WidgetBackground, MenuBackground, Outline,
    DefaultText, DefaultFill, HighlightedText, HighlightedFill, MenuText,
    ComboBoxBackground, TextButtonBackground, ImageButton,
    ControlBackground, SliderTrack, SliderHighlight,
    GraphHigh, GraphLow, EqStart, EqEnd, VerticalSliderLow, PanelBackground,
    NumColors
};

namespace FxPalette
{
// Palette entries are RGB; callers must receive opaque ARGB. Individual controls
// can then apply deliberate transparency, without leaking the old window colour.
constexpr std::uint32_t color(FxColor role, bool dark)
{
    constexpr std::uint32_t darkColors[NumColors] = {
        0x1C1C1E, 0x242426, 0x2C2C2E, 0x545458, 0xE5E5E7, 0x202022, 0xFFFFFF,
        0x3A4B62, 0xF5F5F7, 0x303033, 0x0A84FF, 0x64A8FF, 0x29292C, 0x64A8FF,
        0x64A8FF, 0x64A8FF, 0x43434A, 0x64A8FF, 0x29292C, 0x8E8E93, 0x1C1C1E
    };
    constexpr std::uint32_t lightColors[NumColors] = {
        0xF5F5F7, 0xFFFFFF, 0xFFFFFF, 0xC7C7CC, 0x1D1D1F, 0xF2F2F4, 0x1D1D1F,
        0xD9E9FC, 0x1D1D1F, 0xFFFFFF, 0x007AFF, 0x007AFF, 0xFFFFFF, 0x007AFF,
        0x007AFF, 0x007AFF, 0xFFFFFF, 0x007AFF, 0xFFFFFF, 0x8E8E93, 0xF5F5F7
    };
    return 0xff000000u | (dark ? darkColors[role] : lightColors[role]);
}
}
