// Independent macOS port modifications, 2026-10-29.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "FxPalette.h"

class FxTheme : public juce::LookAndFeel_V4, private juce::DarkModeSettingListener
{
public:
    static constexpr int SLIDER_THUMB_RADIUS = 7;
    static constexpr int ROTARY_SLIDER_THUMB_RADIUS = 5;

    FxTheme();
    ~FxTheme() override;

    juce::LookAndFeel_V4::ColourScheme getFxColourScheme();

    juce::Label* createComboBoxTextBox(juce::ComboBox& box) override;
    juce::Font getComboBoxFont(juce::ComboBox& box) override;
    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override;
    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                      int, int, int, int, juce::ComboBox& box) override;
    void drawComboBoxTextWhenNothingSelected(juce::Graphics&, juce::ComboBox&, juce::Label&) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;
    int getSliderThumbRadius(juce::Slider& slider) override;
    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override;

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool is_separator, bool is_active, bool is_highlighted,
                           bool is_ticked, bool has_submenu, const juce::String& text,
                           const juce::String& shortcut_key_text, const juce::Drawable* icon,
                           const juce::Colour* text_colour) override;
    juce::Font getPopupMenuFont() override;
    void preparePopupMenuWindow(juce::Component& new_window) override;

    juce::Font getTextButtonFont(juce::TextButton&, int button_height) override;
    juce::Font getLabelFont(juce::Label&) override;

    juce::Rectangle<int> getTooltipBounds(const juce::String& tipText,
                                           juce::Point<int> screenPos,
                                           juce::Rectangle<int> parentArea) override;
    void drawTooltip(juce::Graphics&, const juce::String& text, int width, int height) override;

    juce::Font getNormalFont();
    juce::Font getSmallFont();
    juce::Font getTitleFont();

    static juce::uint32 getColor(FxColor color);

private:
    void init();
    void darkModeSettingChanged() override;
    juce::TextLayout layoutTooltipText(const juce::String& text, juce::Colour colour) noexcept;


    juce::Typeface::Ptr font_400_;
    juce::Typeface::Ptr font_600_;
    juce::Typeface::Ptr font_700_;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxTheme)
};

#define FXCOLOR(color) (FxTheme::getColor(FxColor::color))
