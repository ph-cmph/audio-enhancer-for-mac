#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "FxController.h"
#include "PresetSession.h"

class FxEffects : public juce::Component
{
public:
    explicit FxEffects(FxController& controller);
    ~FxEffects() override = default;

    void update();
    void setReference(const PresetValues& values) { reference_ = values; repaint(); }
    std::function<void()> onEdited;

private:
    class FxEffectSlider : public juce::Slider
    {
    public:
        FxEffectSlider(int effect, FxController& controller);
        ~FxEffectSlider() override = default;

        void setEffectValue(float displayValue);
        void enablementChanged() override;

    private:
        void valueChanged() override;
        bool keyPressed(const juce::KeyPress& key) override;

        int effect_;
        FxController& controller_;
    };

    static constexpr int WIDTH       = 168;
    static constexpr int HEIGHT      = 260;
    static constexpr int LABEL_HEIGHT = 16;
    static constexpr int LABEL_GAP    = 0;
    static constexpr int ROW_GAP      = 9;
    static constexpr int SLIDER_WIDTH = 180;
    static constexpr int SLIDER_HEIGHT = 16; // thumb diameter is 16; keeps rows compact for more row spacing
    static constexpr int X_MARGIN    = 8;
    static constexpr int Y_MARGIN    = 36;

    void resized() override;
    void paint(juce::Graphics& g) override;
    void enablementChanged() override;

    FxController& controller_;
    PresetValues reference_;
    std::vector<std::unique_ptr<juce::Label>> labels_;
    std::vector<std::unique_ptr<FxEffectSlider>> effects_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxEffects)
};
