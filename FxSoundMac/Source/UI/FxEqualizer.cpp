// Independent macOS port modifications, 2026-10-28.
// AGPL-3.0-or-later; retain original notices below. See MODIFICATIONS.md and LICENSE.
#include "FxEqualizer.h"
#include "FxTheme.h"

// ──────────────────────────────────────────────────────────── helpers

static juce::String freqLabel(float hz)
{
    if (hz >= 10000.0f)
        return juce::String(juce::roundToInt(hz / 1000.0f)) + "k";
    if (hz >= 1000.0f)
    {
        if (hz == (float)(int)hz && (int)(hz / 1000) * 1000 == (int)hz)
            return juce::String((int)(hz / 1000)) + "k";
        return juce::String(hz / 1000.0f, 1) + "k";
    }
    return juce::String((int)hz);
}

// ──────────────────────────────────────────────────────────── FxEqualizer

FxEqualizer::FxEqualizer(FxController& controller, AudioActivityTap& activity) : controller_(controller), activity_(activity)
{
    // MainComponent calls update() after preparing the DSP and restoring controls.
    // Create sliders there, rather than mutating the component tree during paint.
    setSize(WIDTH, HEIGHT);
    activityLabel_.getProperties().set("useOwnFont", true);
    activityLabel_.setFont(juce::Font(12.0f));
    activityLabel_.setBorderSize(juce::BorderSize<int>(0));
    activityLabel_.setJustificationType(juce::Justification::centredRight);
    activityLabel_.setInterceptsMouseClicks(false, false);
    activityLabel_.setText("Stopped", juce::dontSendNotification);
    addAndMakeVisible(activityLabel_);
}

void FxEqualizer::setProcessingState(bool running)
{
    if (processing_ == running) return;
    processing_ = running;
    configureActivity();
}

void FxEqualizer::setControlsEnabled(bool enabled)
{
    if (controlsEnabled_ == enabled) return;
    controlsEnabled_ = enabled;
    enablementChanged();
}

void FxEqualizer::lookAndFeelChanged() { updateActivityLabel(); repaint(); }

void FxEqualizer::setVisualiserPreferences(bool enabled, bool reducedMotion)
{
    if (visualiserEnabled_ == enabled && reducedMotion_ == reducedMotion) return;
    visualiserEnabled_ = enabled; reducedMotion_ = reducedMotion;
    configureActivity();
}

void FxEqualizer::configureActivity()
{
    spectrum_.reset(); activity_.discard(); wasShowing_ = false;
    if (processing_ && isEnabled() && controlsEnabled_) startTimer(2, 33);
    else stopTimer(2);
    updateActivityLabel();
    repaint();
}

void FxEqualizer::updateActivityLabel()
{
    const auto text = !processing_ || !isEnabled() || !controlsEnabled_ ? "Stopped"
                     : spectrum_.hasAudio() ? "Processing" : "Waiting for audio";
    const auto colour = juce::Colour(FxTheme::getColor(processing_ && isEnabled() && controlsEnabled_ ? FxColor::SliderTrack : FxColor::DefaultText))
        .withAlpha(processing_ && controlsEnabled_ ? 1.0f : 0.6f);
    if (activityLabel_.getText() != text || activityLabel_.findColour(juce::Label::textColourId) != colour)
    {
        activityLabel_.setText(text, juce::dontSendNotification);
        activityLabel_.setColour(juce::Label::textColourId, colour);
        repaint();
    }
}

void FxEqualizer::paintSpectrum(juce::Graphics& g)
{
    if (!processing_ || !isEnabled() || !controlsEnabled_ || !visualiserEnabled_ || reducedMotion_
        || band_boosts_.size() < 2 || band_frequencies_.size() != band_boosts_.size()) return;
    const float base = (float)band_boosts_.front()->getBottom() - FxTheme::SLIDER_THUMB_RADIUS;
    const float top = (float)Y_MARGIN + FxTheme::SLIDER_THUMB_RADIUS;
    for (int band = 0; band < AudioSpectrum::bandCount; ++band)
    {
        const float level = spectrum_.levels()[(size_t)band];
        if (level <= 0.001f) continue;
        const float frequency = AudioSpectrum::bandFrequency(band);
        size_t slot = 0;
        while (slot + 2 < band_frequencies_.size() && frequency > band_frequencies_[slot + 1]) ++slot;
        const float low = juce::jmax(1.0f, band_frequencies_[slot]);
        const float high = juce::jmax(low + 1.0f, band_frequencies_[slot + 1]);
        const float t = juce::jlimit(0.0f, 1.0f, std::log(frequency / low) / std::log(high / low));
        const float x1 = band_boosts_[slot]->getBounds().toFloat().getCentreX();
        const float x2 = band_boosts_[slot + 1]->getBounds().toFloat().getCentreX();
        const float x = x1 + (x2 - x1) * t;
        const float height = (base - top) * level * 0.9f;
        g.setGradientFill(juce::ColourGradient(juce::Colour(FXCOLOR(SliderTrack)).withAlpha(0.3f), x, base - height,
            juce::Colour(FXCOLOR(SliderTrack)).withAlpha(0.035f), x, base, false));
        g.fillRoundedRectangle(x - 3.0f, base - height, 6.0f, height, 2.5f);
    }
}

void FxEqualizer::reinit(int num_bands)
{
    stopTimer(1);
    highlight_mode_ = false;

    for (auto& label : labels_) removeChildComponent(label.get());
    for (auto& slider : band_boosts_) removeChildComponent(slider.get());
    labels_.clear();
    band_boosts_.clear();
    band_gain_values_.clear();
    band_indices_.clear();

    // Cap display to MAX_DISPLAY_BANDS; select evenly-spaced DSP bands across the
    // full frequency range so we cover bass through treble, not just the low end.
    const int display = juce::jmin(num_bands, MAX_DISPLAY_BANDS);

    labels_.resize((size_t)display);
    band_boosts_.resize((size_t)display);
    band_gain_values_.assign((size_t)display, 0.0f);
    band_indices_.resize((size_t)display);
    band_frequencies_.resize((size_t)display);

    for (int i = 0; i < display; ++i)
    {
        // Map display slot i to a DSP band spread evenly across [0, num_bands-1]
        const float t = (display > 1) ? (float)i / (float)(display - 1) : 0.0f;
        band_indices_[i] = (num_bands == display) ? i : juce::roundToInt(t * (float)(num_bands - 1));
    }

    for (int i = 0; i < display; ++i)
    {
        const int dsp_band = band_indices_[i];
        band_frequencies_[(size_t)i] = controller_.getEqBandFrequency(dsp_band);

        labels_[i] = std::make_unique<juce::Label>();
        labels_[i]->getProperties().set("useOwnFont", true);
        labels_[i]->setJustificationType(juce::Justification::centredTop);
        labels_[i]->setText(freqLabel(controller_.getEqBandFrequency(dsp_band)),
                            juce::dontSendNotification);
        addAndMakeVisible(labels_[i].get());

        // Pass the DSP band index so the slider calls setEqBand(dsp_band, ...)
        band_boosts_[i] = std::make_unique<FxEqSlider>(dsp_band, MAX_GAIN, controller_);
        band_boosts_[i]->setSliderStyle(juce::Slider::LinearVertical);
        band_boosts_[i]->setRange(-MAX_GAIN, MAX_GAIN, 1.0);
        band_boosts_[i]->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        band_boosts_[i]->addListener(this);
        addAndMakeVisible(band_boosts_[i].get());
        band_boosts_[i]->setEnabled(isEnabled() && controlsEnabled_);
        labels_[i]->setEnabled(isEnabled() && controlsEnabled_);
    }

    resized();
}

void FxEqualizer::update()
{
    const int numBands = controller_.getNumEqBands();
    if (juce::jmin(numBands, MAX_DISPLAY_BANDS) != (int)band_boosts_.size())
        reinit(numBands);
    for (int i = 0; i < (int)band_boosts_.size(); ++i)
    {
        const int dsp = band_indices_[i];
        band_frequencies_[(size_t)i] = controller_.getEqBandFrequency(dsp);
        band_boosts_[i]->setGainValue(controller_.getEqBand(dsp));
        labels_[i]->setText(freqLabel(controller_.getEqBandFrequency(dsp)), juce::dontSendNotification);
    }
    // Silent slider updates invalidate only the narrow child bounds. The curve
    // belongs to this parent and crosses the gaps, so redraw it as one surface.
    repaint();
}

void FxEqualizer::showValues(bool show)
{
    for (auto& s : band_boosts_)
        s->showValue(show);
}

void FxEqualizer::enablementChanged()
{
    for (int i = 0; i < (int)band_boosts_.size(); ++i)
    {
        band_boosts_[i]->setEnabled(isEnabled() && controlsEnabled_);
        labels_[i]->setEnabled(isEnabled() && controlsEnabled_);
    }
    configureActivity();
    repaint();
}

void FxEqualizer::sliderDragStarted(juce::Slider* slider)
{
    if (highlight_mode_) return;

    if (juce::ModifierKeys::getCurrentModifiersRealtime().isAltDown())
    {
        highlight_mode_ = true;
        startTimer(1, 33);

        for (int i = 0; i < (int)band_boosts_.size(); ++i)
        {
            if (band_boosts_[i].get() != slider)
            {
                band_boosts_[i]->setEnabled(false);
                band_boosts_[i]->showValue(false);
                band_gain_values_[i] = controller_.getEqBand(band_indices_[i]);
            }
        }
    }
}

void FxEqualizer::sliderDragEnded(juce::Slider* slider)
{
    if (!highlight_mode_) return;

    highlight_mode_ = false;

    for (int i = 0; i < (int)band_boosts_.size(); ++i)
    {
        if (band_boosts_[i].get() != slider)
        {
            band_boosts_[i]->setEnabled(true);
            band_boosts_[i]->showValue(true);
            controller_.setEqBand(band_indices_[i], band_gain_values_[i]);
            band_boosts_[i]->setValue(band_gain_values_[i], juce::dontSendNotification);
        }
    }
}

void FxEqualizer::timerCallback(int timerId)
{
    if (timerId == 2)
    {
        if (!isShowing() || (getPeer() != nullptr && getPeer()->isMinimised()))
        { activity_.discard(); spectrum_.reset(); wasShowing_ = false; return; }
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (!wasShowing_) { activity_.discard(); spectrum_.reset(); lastFrameTime_ = now - 33.0; wasShowing_ = true; }
        const auto before = spectrum_.levels();
        spectrum_.update(activity_, (now - lastFrameTime_) / 1000.0, visualiserEnabled_ && !reducedMotion_);
        lastFrameTime_ = now;
        updateActivityLabel();
        if (before != spectrum_.levels()) repaint();
        return;
    }
    if (!highlight_mode_) { stopTimer(1); return; }

    bool done = false;
    for (int i = 0; i < (int)band_boosts_.size(); ++i)
    {
        if (!band_boosts_[i]->isEnabled())
        {
            const float gain    = controller_.getEqBand(band_indices_[i]);
            const float target  = -(MAX_GAIN - 2.0f);
            if (std::abs(gain - target) > 0.5f)
            {
                done = true;
                const float next = gain + (gain < target ? 1.0f : -1.0f);
                controller_.setEqBand(band_indices_[i], next);
                band_boosts_[i]->setValue(next, juce::dontSendNotification);
            }
        }
    }
    if (!done) stopTimer(1);
}

void FxEqualizer::resized()
{
    activityLabel_.setBounds(getWidth() - 166, 6, 150, 20);
    const int num_bands = (int)band_boosts_.size();
    if (num_bands == 0) return;

    const int usable   = getWidth() - X_MARGIN * 2;
    const int band_w   = usable / num_bands;
    const int thumb4   = FxTheme::SLIDER_THUMB_RADIUS * 4;
    // Clamp slider so it never overflows its column
    const int slider_w = juce::jmin(thumb4, band_w);

    // Scale font down as columns narrow
    const float fontSize = (band_w >= 50) ? 12.0f : (band_w >= 30) ? 10.0f : 8.0f;
    // Skip labels to add breathing room between them for very narrow columns:
    //   < 20 px/band → show every 3rd   (~60 px between visible labels)
    //   < 35 px/band → show every 2nd   (~70 px between visible labels)
    //   otherwise    → show all
    const int labelSkip = (band_w < 20) ? 3 : (band_w < 35) ? 2 : 1;

    for (int i = 0; i < num_bands; ++i)
    {
        const int x        = X_MARGIN + i * band_w;
        const int slider_x = x + (band_w - slider_w) / 2;
        band_boosts_[i]->setBounds(slider_x, Y_MARGIN, slider_w, SLIDER_HEIGHT);

        // Remove default border so the full column is available for text;
        // allow horizontal compression before JUCE falls back to "..."
        labels_[i]->setFont(juce::Font(fontSize));
        labels_[i]->setBorderSize(juce::BorderSize<int>(0));
        labels_[i]->setMinimumHorizontalScale(0.5f);
        // Hide intermediate labels — visible labels now have multi-column spacing
        labels_[i]->setVisible(i % labelSkip == 0 || i == num_bands - 1);
        labels_[i]->setBounds(x, band_boosts_[i]->getBottom() + 6, band_w, LABEL_HEIGHT);
    }
}

void FxEqualizer::paint(juce::Graphics& g)
{
    g.setFillType(juce::FillType(juce::Colour(FXCOLOR(ControlBackground)).withAlpha(1.0f)));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 14.0f);
    g.setColour(juce::Colour(FXCOLOR(Outline)).withAlpha(0.3f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 14.0f, 0.7f);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.setColour(juce::Colour(FXCOLOR(DefaultText)).withAlpha(0.7f));
    g.drawText("Equaliser", 16, 6, 140, 20, juce::Justification::centredLeft);
    g.setColour(juce::Colour(FxTheme::getColor(processing_ && controlsEnabled_ ? FxColor::SliderTrack : FxColor::DefaultText)).withAlpha(0.6f));
    const float dotX = (float)activityLabel_.getRight() - activityLabel_.getFont().getStringWidthFloat(activityLabel_.getText()) - 13.0f;
    g.fillEllipse(dotX, 13.0f, 5.0f, 5.0f);
    paintSpectrum(g);

    // Use display count (capped), not the raw DSP band count, for all drawing loops
    const int n = (int)band_boosts_.size();
    if (n < 2) return;

    juce::Colour line_colour = juce::Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f);
    juce::Colour grad_hi     = juce::Colour(FXCOLOR(EqStart)).withAlpha(0.12f);
    juce::Colour grad_lo     = juce::Colour(FXCOLOR(EqEnd)).withAlpha(0.0f);

    if (!isEnabled() || !controlsEnabled_)
    {
        line_colour = line_colour.withSaturation(0.0f);
        grad_hi     = grad_hi.withSaturation(0.0f);
        grad_lo     = grad_lo.withSaturation(0.0f);
    }
    if (highlight_mode_)
        grad_lo = grad_lo.withSaturation(0.0f);

    // Build curve points once — shared by fill and line
    juce::Array<juce::Point<float>> pts;
    for (int i = 0; i < n; ++i)
    {
        const float x = (float)(band_boosts_[i]->getX() + band_boosts_[i]->getWidth() / 2);
        const float y = band_boosts_[i]->getPositionOfValue(band_boosts_[i]->getValue()) + Y_MARGIN;
        pts.add({ x, y });
    }

    // Filled shape under the curve — drawn first so line renders on top
    {
        const float base = (float)(band_boosts_[0]->getBottom() - FxTheme::SLIDER_THUMB_RADIUS);
        juce::Path fill_path;
        fill_path.startNewSubPath(pts[0].x, base);
        for (auto& p : pts)
            fill_path.lineTo(p);
        fill_path.lineTo(pts.getLast().x, base);
        fill_path.closeSubPath();

        const auto gradient = juce::ColourGradient(grad_hi, 0.0f, (float)band_boosts_[0]->getY(),
                                                    grad_lo, 0.0f, (float)band_boosts_[0]->getBottom(), false);
        g.setFillType(juce::FillType(gradient));
        g.fillPath(fill_path);
    }

    // Connecting line on top — fully visible over the fill
    {
        juce::Path line_path;
        for (int i = 0; i < n; ++i)
        {
            if (i == 0) line_path.startNewSubPath(pts[0]);
            else        line_path.lineTo(pts[i]);
        }
        g.setColour(line_colour);
        g.strokePath(line_path, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    }
}

// ──────────────────────────────────────────────────────────── FxEqSlider

FxEqualizer::FxEqSlider::FxEqSlider(int band, float max_gain, FxController& controller)
    : band_(band), controller_(controller)
{
    (void)max_gain;
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);

    gain_label_.getProperties().set("useOwnFont", true);
    gain_label_.setFont(juce::Font(10.5f));
    gain_label_.setJustificationType(juce::Justification::centred);
    gain_label_.setBorderSize(juce::BorderSize<int>(0));
    gain_label_.setMinimumHorizontalScale(0.5f);
    gain_label_.setInterceptsMouseClicks(false, false);
    addChildComponent(gain_label_);
}

void FxEqualizer::FxEqSlider::setGainValue(float db)
{
    setValue(db, juce::dontSendNotification);

    gain_label_.setText(juce::String::formatted(db == 0.0f ? "%.0f" : "%+.0f", db),
                        juce::dontSendNotification);
    const int y = (int)(getPositionOfValue(db)) - FxTheme::SLIDER_THUMB_RADIUS * 3;
    gain_label_.setBounds(gain_label_.getBounds().withY(y));
}

void FxEqualizer::FxEqSlider::showValue(bool show)
{
    gain_label_.setVisible(show && isEnabled());
}

void FxEqualizer::FxEqSlider::enablementChanged()
{
    setMouseCursor(isEnabled() ? juce::MouseCursor::PointingHandCursor
                               : juce::MouseCursor::NormalCursor);
    if (!isEnabled())
        gain_label_.setVisible(false);
}

void FxEqualizer::FxEqSlider::resized()
{
    juce::Slider::resized();
    gain_label_.setBounds(gain_label_.getX(), gain_label_.getY(), getWidth(), LABEL_HEIGHT);
}

void FxEqualizer::FxEqSlider::valueChanged()
{
    const float db = (float)getValue();
    if (std::abs(db - controller_.getEqBand(band_)) > 0.001f)
    {
        controller_.setEqBand(band_, db);

        gain_label_.setText(juce::String::formatted(db == 0.0f ? "%.0f" : "%+.0f", db),
                            juce::dontSendNotification);
        const int y = (int)(getPositionOfValue(db)) - FxTheme::SLIDER_THUMB_RADIUS * 3;
        gain_label_.setBounds(gain_label_.getBounds().withY(y));
    }

    if (auto* parent = getParentComponent())
        parent->repaint();
}

bool FxEqualizer::FxEqSlider::keyPressed(const juce::KeyPress& key)
{
    if (!isEnabled()) return false;

    if (key.isKeyCode(juce::KeyPress::upKey))   { setValue(getValue() + getInterval()); return true; }
    if (key.isKeyCode(juce::KeyPress::downKey))  { setValue(getValue() - getInterval()); return true; }
    return false;
}

void FxEqualizer::FxEqSlider::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
        setValue(0.0, juce::sendNotification);
    else
        juce::Slider::mouseDown(e);
}
