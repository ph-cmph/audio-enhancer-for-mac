// Independent macOS port modifications, 2026-10-29.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include "FxTheme.h"

FxTheme::FxTheme() : LookAndFeel_V4()
{
    init();
    juce::Desktop::getInstance().addDarkModeSettingListener(this);
}

FxTheme::~FxTheme() { juce::Desktop::getInstance().removeDarkModeSettingListener(this); }

void FxTheme::darkModeSettingChanged()
{
    init();
    auto& desktop = juce::Desktop::getInstance();
    for (int i = 0; i < desktop.getNumComponents(); ++i)
        desktop.getComponent(i)->sendLookAndFeelChange();
}

void FxTheme::init()
{
    setColourScheme(getFxColourScheme());

    setColour(juce::ComboBox::arrowColourId,                  juce::Colour(FXCOLOR(ImageButton)).withAlpha(1.0f));
    setColour(juce::ComboBox::backgroundColourId,              juce::Colour(FXCOLOR(ComboBoxBackground)).withAlpha(1.0f));
    setColour(juce::ComboBox::outlineColourId,                 juce::Colour(FXCOLOR(ComboBoxBackground)).withAlpha(1.0f));
    setColour(juce::ComboBox::focusedOutlineColourId,          juce::Colour(FXCOLOR(SliderHighlight)).withAlpha(0.2f));
    setColour(juce::ComboBox::textColourId,                    juce::Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
    setColour(juce::TextEditor::backgroundColourId,            juce::Colour(FXCOLOR(DefaultFill)).withAlpha(1.0f));
    setColour(juce::TextEditor::outlineColourId,               juce::Colour(FXCOLOR(DefaultFill)).withAlpha(1.0f));
    setColour(juce::TextEditor::focusedOutlineColourId,        juce::Colour(FXCOLOR(DefaultFill)).withAlpha(1.0f));
    setColour(juce::TextEditor::textColourId,                  juce::Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
    setColour(juce::TextEditor::highlightedTextColourId,       juce::Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f));
    setColour(juce::TextButton::buttonColourId,                juce::Colour(FXCOLOR(TextButtonBackground)).withAlpha(1.0f));
    setColour(juce::TextButton::buttonOnColourId,              juce::Colour(FXCOLOR(TextButtonBackground)).withAlpha(1.0f));
    setColour(juce::TextButton::textColourOffId,               juce::Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f));
    setColour(juce::TextButton::textColourOnId,                juce::Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f));
    setColour(juce::HyperlinkButton::textColourId,             juce::Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f));
    setColour(juce::CaretComponent::caretColourId,             juce::Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
    setColour(juce::PopupMenu::backgroundColourId,             juce::Colour(FXCOLOR(WidgetBackground)).withAlpha(1.0f));
    setColour(juce::PopupMenu::highlightedBackgroundColourId,  juce::Colour(FXCOLOR(ImageButton)).withAlpha(1.0f));
    setColour(juce::Slider::rotarySliderOutlineColourId,       juce::Colour(FXCOLOR(SliderTrack)).withAlpha(0.2f));
    setColour(juce::Slider::rotarySliderFillColourId,          juce::Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));
    setColour(juce::ScrollBar::thumbColourId,                  juce::Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f));
    setColour(juce::Label::textColourId,                       juce::Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));

}

juce::LookAndFeel_V4::ColourScheme FxTheme::getFxColourScheme()
{
    return {
        juce::Colour(FXCOLOR(WindowBackground)).withAlpha(1.0f),
        juce::Colour(FXCOLOR(WidgetBackground)).withAlpha(1.0f),
        juce::Colour(FXCOLOR(MenuBackground)).withAlpha(1.0f),
        juce::Colour(FXCOLOR(Outline)).withAlpha(1.0f),
        juce::Colour(FXCOLOR(DefaultText)).withAlpha(1.0f),
        juce::Colour(FXCOLOR(DefaultFill)).withAlpha(0.2f),
        juce::Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f),
        juce::Colour(FXCOLOR(HighlightedFill)).withAlpha(1.0f),
        juce::Colour(FXCOLOR(MenuText)).withAlpha(1.0f)
    };
}

juce::Label* FxTheme::createComboBoxTextBox(juce::ComboBox& box)
{
    auto* label = LookAndFeel_V4::createComboBoxTextBox(box);
    label->setMouseCursor(juce::MouseCursor::PointingHandCursor);
    return label;
}

juce::Font FxTheme::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(13.0f);
}

void FxTheme::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setMinimumHorizontalScale(1.0f);
    LookAndFeel_V4::positionComboBoxText(box, label);
    label.setBounds(label.getBounds().withX(10));
}

void FxTheme::drawComboBox(juce::Graphics& g, int width, int height, bool,
                            int, int, int, int, juce::ComboBox& box)
{
    const float cornerSize = 8.0f;
    juce::Rectangle<int> boxBounds(0, 0, width, height);

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

    if (box.hasKeyboardFocus(true))
        g.setColour(box.findColour(juce::ComboBox::focusedOutlineColourId));
    else
        g.setColour(juce::Colour(FXCOLOR(Outline)).withAlpha(0.4f));
    g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);

    const float cx = (float)width - 16.0f, cy = (float)height * 0.5f;
    juce::Path chevron; chevron.startNewSubPath(cx - 3, cy - 1.5f);
    chevron.lineTo(cx, cy + 1.5f); chevron.lineTo(cx + 3, cy - 1.5f);
    g.setColour(juce::Colour(FXCOLOR(DefaultText)).withAlpha(box.isEnabled() ? 0.6f : 0.3f));
    g.strokePath(chevron, juce::PathStrokeType(1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void FxTheme::drawComboBoxTextWhenNothingSelected(juce::Graphics& g, juce::ComboBox& box, juce::Label& label)
{
    g.setColour(findColour(juce::ComboBox::textColourId).withMultipliedAlpha(0.5f));
    auto font = label.getLookAndFeel().getLabelFont(label);
    g.setFont(font);
    auto textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds().withX(10));
    g.drawFittedText(box.getTextWhenNothingSelected(), textArea, label.getJustificationType(),
                     juce::jmax(1, (int)((float)textArea.getHeight() / font.getHeight())),
                     label.getMinimumHorizontalScale());
}

void FxTheme::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                float sliderPos, float, float,
                                juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical && style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, 0, 0, style, slider);
        return;
    }
    const bool vertical = style == juce::Slider::LinearVertical;
    const bool enabled = slider.isEnabled();
    const auto accent = juce::Colour(FXCOLOR(SliderTrack)).withAlpha(enabled ? 1.0f : 0.25f);
    const auto outline = juce::Colour(FXCOLOR(Outline)).withAlpha(1.0f);
    const float cx = vertical ? (float)x + (float)width * 0.5f : sliderPos;
    const float cy = vertical ? sliderPos : (float)y + (float)height * 0.5f;
    const float radius = (float)getSliderThumbRadius(slider);
    if (vertical)
    {
        g.setColour(outline.withAlpha(0.25f));
        g.drawLine(cx, (float)y, cx, (float)(y + height), 0.7f);
    }
    else
    {
        g.setColour(outline.withAlpha(0.38f));
        g.fillRoundedRectangle((float)x, cy - 1.75f, (float)width, 3.5f, 1.75f);
        g.setColour(accent);
        g.fillRoundedRectangle((float)x, cy - 1.75f, juce::jlimit(0.0f, (float)width, sliderPos - (float)x), 3.5f, 1.75f);
    }
    if (slider.hasKeyboardFocus(true) || slider.getThumbBeingDragged() >= 0)
    {
        g.setColour(accent.withAlpha(0.13f));
        g.fillEllipse(cx - radius - 3, cy - radius - 3, (radius + 3) * 2, (radius + 3) * 2);
    }
    g.setColour(juce::Colours::black.withAlpha(enabled ? 0.09f : 0.03f));
    g.fillEllipse(cx - radius, cy - radius + 1.0f, radius * 2, radius * 2);
    g.setColour(juce::Colours::white.withAlpha(enabled ? 1.0f : 0.7f));
    g.fillEllipse(cx - radius, cy - radius, radius * 2, radius * 2);
    g.setColour(outline.withAlpha(enabled ? 0.55f : 0.25f));
    g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, 0.7f);
}

void FxTheme::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                juce::Slider& slider)
{
    auto outline = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
    auto fill    = slider.findColour(juce::Slider::rotarySliderFillColourId);

    if (!slider.isEnabled())
    {
        outline = outline.withSaturation(0.0f);
        fill    = fill.withSaturation(0.0f);
    }

    auto bounds  = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
    auto radius  = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW   = 5.0f;
    auto arcRadius = radius - lineW * 0.5f;

    juce::Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(),
                                 arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(outline);
    g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(),
                            arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);
    g.setColour(fill);
    g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto thumbRadius = (float)getSliderThumbRadius(slider);
    juce::Point<float> thumbPoint(
        bounds.getCentreX() + arcRadius * std::cos(toAngle - juce::MathConstants<float>::halfPi),
        bounds.getCentreY() + arcRadius * std::sin(toAngle - juce::MathConstants<float>::halfPi));

    g.setColour(juce::Colours::white.withAlpha(slider.isEnabled() ? 1.0f : 0.5f));
    g.fillEllipse(thumbPoint.getX() - thumbRadius, thumbPoint.getY() - thumbRadius,
                  thumbRadius * 2, thumbRadius * 2);

}

int FxTheme::getSliderThumbRadius(juce::Slider& slider)
{
    return (slider.getSliderStyle() == juce::Slider::Rotary) ? ROTARY_SLIDER_THUMB_RADIUS : SLIDER_THUMB_RADIUS;
}

juce::Slider::SliderLayout FxTheme::getSliderLayout(juce::Slider& slider)
{
    auto layout = LookAndFeel_V4::getSliderLayout(slider);
    auto style  = slider.getSliderStyle();

    if (style == juce::Slider::LinearVertical)
    {
        layout.sliderBounds.setY(layout.sliderBounds.getY() + SLIDER_THUMB_RADIUS * 2);
        layout.sliderBounds.setHeight(layout.sliderBounds.getHeight() - SLIDER_THUMB_RADIUS * 2);
    }
    else if (style == juce::Slider::LinearHorizontal)
    {
        layout.sliderBounds.setWidth(layout.sliderBounds.getWidth() - SLIDER_THUMB_RADIUS * 4);
    }
    return layout;
}

void FxTheme::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                 bool is_separator, bool is_active, bool is_highlighted,
                                 bool is_ticked, bool has_submenu, const juce::String& text,
                                 const juce::String& shortcut_key_text, const juce::Drawable* icon,
                                 const juce::Colour* text_colour)
{
    LookAndFeel_V4::drawPopupMenuItem(g, area, is_separator, is_active,
                                      is_highlighted || is_ticked, is_ticked,
                                      has_submenu, text, shortcut_key_text, icon, text_colour);
    if (is_ticked)
    {
        g.setColour(findColour(juce::PopupMenu::textColourId).withAlpha(1.0f));
        g.drawRect(area.toFloat());
    }
}

juce::Font FxTheme::getPopupMenuFont()
{
    return juce::Font(13.0f);
}

void FxTheme::preparePopupMenuWindow(juce::Component& new_window)
{
    new_window.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    for (auto* child : new_window.getChildren())
        child->setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

juce::Font FxTheme::getTextButtonFont(juce::TextButton&, int button_height)
{
    return juce::Font(juce::jmin(13.0f, (float)button_height));
}

juce::Font FxTheme::getLabelFont(juce::Label& label)
{
    if ((bool)label.getProperties().getWithDefault("useOwnFont", false)) return label.getFont();
    return getNormalFont();
}

juce::Font FxTheme::getNormalFont()
{
    return juce::Font(13.0f);
}

juce::Font FxTheme::getSmallFont()
{
    return juce::Font(11.0f);
}

juce::Font FxTheme::getTitleFont()
{
    return juce::Font(15.0f);
}

juce::uint32 FxTheme::getColor(FxColor color)
{
    return FxPalette::color(color, juce::Desktop::getInstance().isDarkModeActive());
}

juce::Rectangle<int> FxTheme::getTooltipBounds(const juce::String& tipText,
                                                 juce::Point<int> screenPos,
                                                 juce::Rectangle<int> parentArea)
{
    const auto tl = layoutTooltipText(tipText, juce::Colours::black);
    auto w = (int)(tl.getWidth() + 20.0f);
    auto h = (int)(tl.getHeight() + 12.0f);
    return juce::Rectangle<int>(
        screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 18) : screenPos.x + 36,
        screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 12) : screenPos.y + 12,
        w, h).constrainedWithin(parentArea);
}

void FxTheme::drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height)
{
    juce::Rectangle<int> bounds(width, height);
    g.setColour(findColour(juce::TooltipWindow::backgroundColourId));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
    g.setColour(findColour(juce::TooltipWindow::outlineColourId));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f, 0.5f), 5.0f, 1.0f);
    layoutTooltipText(text, findColour(juce::TooltipWindow::textColourId))
        .draw(g, bounds.toFloat().reduced(10.0f, 0.0f));
}

juce::TextLayout FxTheme::layoutTooltipText(const juce::String& text, juce::Colour colour) noexcept
{
    juce::AttributedString s;
    s.setWordWrap(juce::AttributedString::byWord);
    s.setJustification(juce::Justification::centredLeft);
    s.append(text, getNormalFont().withHeight(14.0f), colour);
    juce::TextLayout tl;
    tl.createLayout(s, 400.0f);
    return tl;
}
