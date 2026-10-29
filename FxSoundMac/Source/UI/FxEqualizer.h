#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "FxController.h"
#include "PresetSession.h"
#include "../Audio/AudioActivity.h"

class FxEqualizer : public juce::Component,
                    public juce::Slider::Listener,
                    private juce::MultiTimer
{
public:
    FxEqualizer(FxController& controller, AudioActivityTap& activity);
    ~FxEqualizer() override = default;

    void reinit(int num_bands);
    void update();
    void setReference(const PresetValues& values) { reference_ = values; repaint(); }
    std::function<void()> onEdited;
    void setProcessingState(bool running);
    void setControlsEnabled(bool enabled);
    void setVisualiserPreferences(bool enabled, bool reducedMotion);

    void sliderValueChanged(juce::Slider*) override { repaint(); if (onEdited) onEdited(); }
    void sliderDragStarted(juce::Slider* slider) override;
    void sliderDragEnded(juce::Slider* slider) override;
    void timerCallback(int timerId) override;

private:
    class FxEqSlider : public juce::Slider
    {
    public:
        FxEqSlider(int band, float max_gain, FxController& controller);
        ~FxEqSlider() override = default;

        void setGainValue(float db);
        void enablementChanged() override;

    private:
        void valueChanged() override;
        bool keyPressed(const juce::KeyPress& key) override;
        void mouseDown(const juce::MouseEvent& e) override;

        int band_;
        FxController& controller_;
    };

    static constexpr int WIDTH             = 776;
    static constexpr int HEIGHT            = 260;
    static constexpr int SLIDER_HEIGHT     = 150;
    static constexpr int LABEL_HEIGHT      = 16;  // room for freq label so it isn't vertically clipped
    static constexpr int X_MARGIN         = 16;
    static constexpr int Y_MARGIN         = 44;
    static constexpr float MAX_GAIN       = 12.0f;
    static constexpr int MAX_DISPLAY_BANDS = 10; // cap display to match Windows default

    void resized() override;
    void paint(juce::Graphics& g) override;
    void enablementChanged() override;
    void lookAndFeelChanged() override;
    void configureActivity();
    void updateActivityLabel();
    void paintSpectrum(juce::Graphics&);

    FxController& controller_;
    PresetValues reference_;
    AudioActivityTap& activity_;
    AudioSpectrum spectrum_;
    juce::Label activityLabel_;
    bool processing_ = false, visualiserEnabled_ = true, reducedMotion_ = false;
    bool controlsEnabled_ = true;
    bool wasShowing_ = false;
    double lastFrameTime_ = 0;
    std::vector<float> band_frequencies_;
    std::vector<std::unique_ptr<juce::Label>> labels_;
    std::vector<std::unique_ptr<FxEqSlider>> band_boosts_;
    std::vector<float> band_gain_values_;
    std::vector<int>   band_indices_;  // display slot i → actual DSP band index
    bool highlight_mode_ { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxEqualizer)
};
