#include "FxEffects.h"
#include "FxTheme.h"
#include "DfxDsp.h"
#include "ValueReadout.h"

namespace {
const char* kEffectNames[DfxDsp::NumEffects] = {
    "Clarity", "Ambience", "Surround Sound", "Dynamic Boost", "Bass Boost"
};
} // namespace

// ──────────────────────────────────────────────────────────── FxEffects

FxEffects::FxEffects(FxController& controller) : controller_(controller)
{
    labels_.resize(DfxDsp::NumEffects);
    effects_.resize(DfxDsp::NumEffects);

    for (int i = 0; i < DfxDsp::NumEffects; ++i)
    {
        labels_[i] = std::make_unique<juce::Label>(kEffectNames[i], kEffectNames[i]);
        labels_[i]->setJustificationType(juce::Justification::topLeft);
        auto border = labels_[i]->getBorderSize();
        border.setLeft(0);
        labels_[i]->setBorderSize(border);
        addAndMakeVisible(labels_[i].get());

        effects_[i] = std::make_unique<FxEffectSlider>(i, controller_);
        effects_[i]->setName(kEffectNames[i]);
        effects_[i]->setSliderStyle(juce::Slider::LinearHorizontal);
        effects_[i]->setRange(0.0, 10.0, 0.1);
        effects_[i]->setTitle(kEffectNames[i]);
        effects_[i]->onValueChange = [this] { repaint(); if (onEdited) onEdited(); };
        effects_[i]->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(effects_[i].get());
    }

    setSize(WIDTH, HEIGHT);
}

void FxEffects::update()
{
    for (int i = 0; i < DfxDsp::NumEffects; ++i)
    {
        const float value = controller_.getEffect(static_cast<DfxDsp::Effect>(i));
        effects_[i]->setEffectValue(value * 10.0f);
    }
}

void FxEffects::enablementChanged()
{
    for (int i = 0; i < DfxDsp::NumEffects; ++i)
    {
        labels_[i]->setEnabled(isEnabled());
        effects_[i]->setEnabled(isEnabled());
    }
    repaint();
}

void FxEffects::resized()
{
    int y = Y_MARGIN;
    for (int i = 0; i < DfxDsp::NumEffects; ++i)
    {
        labels_[i]->setBounds(15, y, 116, LABEL_HEIGHT);
        effects_[i]->setBounds(X_MARGIN, labels_[i]->getBottom() + LABEL_GAP, getWidth() - X_MARGIN * 2, SLIDER_HEIGHT);
        y = effects_[i]->getBottom() + ROW_GAP;
    }
}

void FxEffects::paint(juce::Graphics& g)
{
    g.setFillType(juce::FillType(juce::Colour(FXCOLOR(ControlBackground)).withAlpha(1.0f)));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 14.0f);
    g.setColour(juce::Colour(FXCOLOR(Outline)).withAlpha(0.3f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 14.0f, 0.7f);
    g.setColour(juce::Colour(FXCOLOR(DefaultText)).withAlpha(0.7f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("Enhancements", 16, 9, getWidth() - 32, 18, juce::Justification::centredLeft);
    for (int i = 0; i < DfxDsp::NumEffects; ++i)
        ValueReadout::draw(g, {132, labels_[i]->getY(), getWidth() - 148, 16},
            controller_.getEffect(static_cast<DfxDsp::Effect>(i)) * 10.0f,
            reference_.effects[static_cast<size_t>(i)] * 10.0f, false, isEnabled(), false);
}

// ──────────────────────────────────────────────────────── FxEffectSlider

FxEffects::FxEffectSlider::FxEffectSlider(int effect, FxController& controller)
    : effect_(effect), controller_(controller)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);

}

void FxEffects::FxEffectSlider::setEffectValue(float displayValue)
{
    setValue(displayValue, juce::dontSendNotification);
}

void FxEffects::FxEffectSlider::enablementChanged()
{
    setMouseCursor(isEnabled() ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void FxEffects::FxEffectSlider::valueChanged()
{
    controller_.setEffect(static_cast<DfxDsp::Effect>(effect_), static_cast<float>(getValue()) / 10.0f);
}

bool FxEffects::FxEffectSlider::keyPressed(const juce::KeyPress& key)
{
    if (!isEnabled()) return false;

    if (key.isKeyCode(juce::KeyPress::upKey))
    {
        setValue(getValue() + getInterval());
        return true;
    }
    if (key.isKeyCode(juce::KeyPress::downKey))
    {
        setValue(getValue() - getInterval());
        return true;
    }
    return false;
}
