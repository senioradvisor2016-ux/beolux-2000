/*  BCLookAndFeel — Ferroflux look (cream rotaries, piano-key buttons,
    dark-recess combos). */

#include "BCLookAndFeel.h"
#include "BinaryData.h"
#include "AssetRegistry.h"

namespace bc2000dl::ui
{
    using namespace bc2000dl::ui::colours;

    BCLookAndFeel::BCLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, kInk);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, kInkSoft);

        setColour (juce::ComboBox::backgroundColourId, kRecessTop);
        setColour (juce::ComboBox::textColourId, kAmber);
        setColour (juce::ComboBox::outlineColourId, juce::Colours::black);
        setColour (juce::ComboBox::arrowColourId, kAmber);
        setColour (juce::PopupMenu::backgroundColourId, kRecessTop);
        setColour (juce::PopupMenu::textColourId, kAmber);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, kAmberDim);
        setColour (juce::PopupMenu::highlightedTextColourId, kAmberGlow);
    }

    juce::Font BCLookAndFeel::getLabelFont (juce::Label& label)
    {
        return juce::Font (juce::FontOptions (label.getFont().getHeight()).withName ("Helvetica"));
    }

    juce::Label* BCLookAndFeel::createSliderTextBox (juce::Slider& slider)
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (slider);
        l->setColour (juce::Label::textColourId, kInk);
        l->setFont (juce::Font (juce::FontOptions (9.0f).withName ("Helvetica")));
        return l;
    }

    namespace
    {
        constexpr int kKnobFrames = 60;
        juce::Image& photoKnobImage() { return AssetRegistry::image (Asset::KnobMetal); }

        // Använd default chrome-dome filmstrip för ALLA storlekar (v18-stil som funkade)
        // — big/medium-versionerna hade off-center crop som gav skev placering
        juce::Image& knobFilmstripFor (float /*outerR*/)
        {
            return AssetRegistry::image (Asset::KnobFilmstrip);
        }
    }

    void BCLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                          int x, int y, int w, int h,
                                          float sliderPos,
                                          float rotaryStart,
                                          float rotaryEnd,
                                          juce::Slider& slider)
    {
        const auto bounds = juce::Rectangle<float> (
            (float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float outerR = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f - 3.0f;

        // === UAD-style focus-glow (orange = hover, green = active drag) ===
        const bool isDragging = slider.isMouseButtonDown();
        const bool isHover    = slider.isMouseOver() || isDragging;
        if (isHover)
        {
            const auto glowColor = isDragging
                ? juce::Colour (0xff60c850)       // active green
                : juce::Colour (0xffe88030);      // hover orange
            const float gcx = bounds.getCentreX();
            const float gcy = bounds.getCentreY();
            const float glowR = outerR * 1.18f;
            // 3-pass alpha rings ger soft glow
            for (int i = 0; i < 3; ++i)
            {
                g.setColour (glowColor.withAlpha (0.14f - i * 0.04f));
                const float r = glowR + i * 1.5f;
                g.drawEllipse (gcx - r, gcy - r, r * 2, r * 2, 1.5f);
            }
        }
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float angle = rotaryStart + sliderPos * (rotaryEnd - rotaryStart);

        // ===== Noisehead-style knob: FLUSH mot surface (ingen drop-shadow / upphöjning) =====

        // (Drop-shadow borttagen — knob ligger direkt mot panelen)

        // 2. Statiska tick-marks runt om (Noisehead-style: dense radial)
        // 41 ticks över 270°, längre vid 0/+12/-12, kortare däremellan
        const int nTicks = 41;
        const float tickSpan = juce::MathConstants<float>::twoPi * 0.75f;
        const float tickInner = outerR * 1.02f;     // strax utanför chrome-cap
        const float tickOuterShort = outerR * 1.08f;
        const float tickOuterLong  = outerR * 1.13f;

        // 2.5 — Numeric scale 0-10 runt knob OM den är stor nog (>= 22px radie)
        const bool isBigKnob = outerR >= 22.0f;
        if (isBigKnob)
        {
            const float labelR = outerR * 1.30f;
            const float fontSize = juce::jmap (outerR, 22.0f, 60.0f, 7.0f, 10.0f);
            for (int i = 0; i <= 10; ++i)
            {
                const float ta = rotaryStart + tickSpan * (i / 10.0f);
                const float lx = cx + std::sin (ta) * labelR;
                const float ly = cy - std::cos (ta) * labelR;
                g.setColour (juce::Colour (0xffd8d5cc).withAlpha (0.95f));
                g.setFont (juce::Font (juce::FontOptions (fontSize).withName ("Helvetica").withStyle ("Bold")));
                g.drawText (juce::String (i),
                            juce::Rectangle<float> (lx - 8, ly - 5, 16, 10),
                            juce::Justification::centred, false);
            }
        }
        for (int i = 0; i < nTicks; ++i)
        {
            const float ta = rotaryStart + tickSpan * i / (float) (nTicks - 1);
            const bool isMajor = (i % 5 == 0);
            const float r2 = isMajor ? tickOuterLong : tickOuterShort;
            const float tx1 = cx + std::sin (ta) * tickInner;
            const float ty1 = cy - std::cos (ta) * tickInner;
            const float tx2 = cx + std::sin (ta) * r2;
            const float ty2 = cy - std::cos (ta) * r2;
            g.setColour (juce::Colour (0xffd0cdc4).withAlpha (isMajor ? 0.85f : 0.55f));
            g.drawLine (tx1, ty1, tx2, ty2, isMajor ? 1.4f : 0.9f);
        }

        // 3. Aktiv-värde tick-highlight (lyser upp tickerna fram till sliderPos)
        const int activeTicks = (int) std::round (sliderPos * (nTicks - 1));
        for (int i = 0; i <= activeTicks; ++i)
        {
            const float ta = rotaryStart + tickSpan * i / (float) (nTicks - 1);
            const float r2 = (i % 5 == 0) ? tickOuterLong : tickOuterShort;
            const float tx1 = cx + std::sin (ta) * tickInner;
            const float ty1 = cy - std::cos (ta) * tickInner;
            const float tx2 = cx + std::sin (ta) * r2;
            const float ty2 = cy - std::cos (ta) * r2;
            // Värme-glöd från amber (start) till grön (full)
            const float t = activeTicks > 0 ? (float) i / (float) (nTicks - 1) : 0.0f;
            const auto glow = juce::Colour (0xffe88030).interpolatedWith (
                                  juce::Colour (0xff60c850), t);
            g.setColour (glow.withAlpha (0.95f));
            g.drawLine (tx1, ty1, tx2, ty2, (i % 5 == 0) ? 1.6f : 1.1f);
        }

        // 4. Filmstrip-rendering: välj rätt storlek + frame baserat på sliderPos (0..1)
        auto& strip = knobFilmstripFor (outerR);
        if (strip.isValid())
        {
            const int frameSize = strip.getWidth();          // 256
            const int frameIdx  = juce::jlimit (0, kKnobFrames - 1,
                                                 (int) std::round (sliderPos * (kKnobFrames - 1)));
            const int srcY      = frameIdx * frameSize;
            const float diameter = outerR * 2.0f;

            g.drawImage (strip,
                          (int) (cx - outerR), (int) (cy - outerR),
                          (int) diameter, (int) diameter,
                          0, srcY, frameSize, frameSize);
        }
        else if (photoKnobImage().isValid())
        {
            // Fallback 1: enkel foto-knob med rotation (utan filmstrip-cache)
            auto& knobImg = photoKnobImage();
            juce::Graphics::ScopedSaveState s (g);
            const float halfImg = (float) knobImg.getWidth() * 0.5f;
            const float scale = (outerR * 2.0f) / (float) knobImg.getWidth();
            g.addTransform (juce::AffineTransform::translation (-halfImg, -halfImg)
                              .scaled (scale).rotated (angle).translated (cx, cy));
            g.drawImage (knobImg, 0, 0, knobImg.getWidth(), knobImg.getHeight(),
                          0, 0, knobImg.getWidth(), knobImg.getHeight());
        }
        else
        {
            // Fallback 2: procedural chrome
            juce::ColourGradient capGrad (
                juce::Colour (0xfff5f2ea), cx - outerR * 0.4f, cy - outerR * 0.5f,
                juce::Colour (0xff4a4842), cx + outerR * 0.6f, cy + outerR * 0.7f, true);
            g.setGradientFill (capGrad);
            g.fillEllipse (cx - outerR, cy - outerR, outerR * 2, outerR * 2);
        }
    }

    void BCLookAndFeel::drawLinearSlider (juce::Graphics& g,
                                          int x, int y, int w, int h,
                                          float sliderPos,
                                          float minPos, float maxPos,
                                          juce::Slider::SliderStyle style,
                                          juce::Slider& slider)
    {
        juce::ignoreUnused (minPos, maxPos, slider);

        const auto bounds = juce::Rectangle<float> (
            (float) x, (float) y, (float) w, (float) h);

        if (style == juce::Slider::LinearVertical || style == juce::Slider::LinearBarVertical)
        {
            // Vertikalt slot — slim på mörk recess
            const float trackW = 4.0f;
            const float trackX = bounds.getCentreX() - trackW * 0.5f;

            juce::ColourGradient trackGrad (
                juce::Colour (0xff050403), trackX, bounds.getY(),
                juce::Colour (0xff1a140d), trackX, bounds.getBottom(), false);
            g.setGradientFill (trackGrad);
            g.fillRoundedRectangle (trackX, bounds.getY() + 2, trackW,
                                     bounds.getHeight() - 4, 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.drawRoundedRectangle (trackX, bounds.getY() + 2, trackW,
                                     bounds.getHeight() - 4, 2.0f, 0.5f);

            // Tick-marks (11 streck) — vänster + höger
            for (int i = 0; i < 11; ++i)
            {
                const float ty = bounds.getY() + 4 + (bounds.getHeight() - 8) * (i / 10.0f);
                const bool isEdge = (i == 0 || i == 10);
                g.setColour (isEdge ? kInk : kInkSoft);
                g.fillRect (trackX - 8.0f, ty - 0.5f, 5.0f, 1.0f);
                g.fillRect (trackX + trackW + 3.0f, ty - 0.5f, 5.0f, 1.0f);
            }

            // Fill ovanför thumb (amber-glow)
            const float fillTopY = sliderPos;
            juce::ColourGradient fillGrad (
                kAmber.withAlpha (0.40f), trackX - 1, fillTopY,
                kAmber.withAlpha (0.10f), trackX - 1, bounds.getBottom(), false);
            g.setGradientFill (fillGrad);
            g.fillRoundedRectangle (trackX - 1, fillTopY, trackW + 2,
                                     bounds.getBottom() - fillTopY - 2, 2.0f);

            // Thumb (mörk knab med amber center-line)
            const float thumbW = bounds.getWidth() * 0.78f;
            const float thumbH = 14.0f;
            const float thumbX = bounds.getCentreX() - thumbW * 0.5f;
            const float thumbY = sliderPos - thumbH * 0.5f;

            juce::ColourGradient thumbGrad (
                juce::Colour (0xff3a3128), thumbX, thumbY,
                juce::Colour (0xff0d0a07), thumbX, thumbY + thumbH, false);
            thumbGrad.addColour (0.5, juce::Colour (0xff1a1410));
            g.setGradientFill (thumbGrad);
            g.fillRoundedRectangle (thumbX, thumbY, thumbW, thumbH, 2.0f);

            // Top-highlight
            g.setColour (kKnabSatin.withAlpha (0.3f));
            g.drawLine (thumbX + 1, thumbY + 1, thumbX + thumbW - 1, thumbY + 1, 0.5f);

            // Grip-lines (3)
            for (int i = 0; i < 3; ++i)
            {
                const float ly = thumbY + 4.0f + i * 2.5f;
                g.setColour (juce::Colour (0xff8a7a52).withAlpha (0.5f));
                g.fillRect (thumbX + 4.0f, ly, thumbW - 8.0f, 0.6f);
            }

            // Center index-line (amber LED)
            g.setColour (kAmber);
            g.fillRect (thumbX - 3.0f, sliderPos - 0.75f, thumbW + 6.0f, 1.5f);
            g.setColour (kAmberGlow.withAlpha (0.6f));
            g.fillRect (thumbX - 4.0f, sliderPos - 1.25f, thumbW + 8.0f, 2.5f);
            g.setColour (kAmber);
            g.fillRect (thumbX - 3.0f, sliderPos - 0.75f, thumbW + 6.0f, 1.5f);
        }
        else
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos,
                                              minPos, maxPos, style, slider);
        }
    }

    void BCLookAndFeel::drawToggleButton (juce::Graphics& g,
                                          juce::ToggleButton& button,
                                          bool isHighlighted,
                                          bool isDown)
    {
        // BC2000 DL-stil: små grå kvadratiska push-knappar.
        // Ljusgrå satin-yta i opress, mörkare i pressed (mekanisk-tangent-feel).
        // Liten röd LED-prick uppe till vänster vid aktiv (matchar B&O foto).
        const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        const bool isOn = button.getToggleState();
        const bool pressed = isOn || isDown;

        if (pressed)
        {
            // Indragen mörkare grå-tangent
            juce::ColourGradient g1 (
                juce::Colour (0xff5a5752), bounds.getX(), bounds.getY(),
                juce::Colour (0xff3a3832), bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill (g1);
            g.fillRoundedRectangle (bounds, 1.5f);

            // Inner skugga för "press"
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.drawLine (bounds.getX() + 1, bounds.getY() + 1.5f,
                        bounds.getRight() - 1, bounds.getY() + 1.5f, 1.0f);
        }
        else
        {
            // Up — ljusgrå satin (kromad B&O-stil)
            juce::ColourGradient g1 (
                juce::Colour (0xffd0cdc4), bounds.getX(), bounds.getY(),
                juce::Colour (0xff96938a), bounds.getX(), bounds.getBottom(), false);
            g1.addColour (0.5, juce::Colour (0xffb8b5ac));
            g.setGradientFill (g1);
            g.fillRoundedRectangle (bounds, 1.5f);

            // Top-highlight (ljus reflektion)
            g.setColour (juce::Colours::white.withAlpha (0.25f));
            g.drawLine (bounds.getX() + 1, bounds.getY() + 0.5f,
                        bounds.getRight() - 1, bounds.getY() + 0.5f, 0.8f);
        }

        // Border (mörk)
        g.setColour (juce::Colour (0xff141210));
        g.drawRoundedRectangle (bounds, 1.5f, 0.7f);

        // Hover-tint
        if (isHighlighted && ! pressed)
        {
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.fillRoundedRectangle (bounds, 1.5f);
        }

        // Aktiv-LED (röd punkt uppe vänster — matchar BC2000 foto)
        if (isOn)
        {
            const float lx = bounds.getX() + 4.0f;
            const float ly = bounds.getY() + 4.0f;
            g.setColour (juce::Colour (0xffd8442a).withAlpha (0.5f));
            g.fillEllipse (lx - 1, ly - 1, 5, 5);
            g.setColour (juce::Colour (0xffff5a3a));
            g.fillEllipse (lx, ly, 3, 3);
        }

        // Text — engraved svart för matchande B&O-typografi
        g.setColour (juce::Colour (0xff141210));
        g.setFont (juce::Font (juce::FontOptions (8.0f).withName ("Helvetica").withStyle ("Bold")));
        g.drawText (button.getButtonText(), bounds.translated (0, pressed ? 1 : 0),
                    juce::Justification::centred, false);
    }

    namespace
    {
        juce::Image& photoButtonImage()       { return AssetRegistry::image (Asset::BtnRect); }
        juce::Image& photoButtonAccentImage() { return AssetRegistry::image (Asset::BtnAccent); }
    }

    void BCLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                              juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool isHighlighted,
                                              bool isDown)
    {
        juce::ignoreUnused (backgroundColour);

        const auto bounds = button.getLocalBounds().toFloat();
        const bool engaged = button.getToggleState() || isDown;
        const float radius = 2.5f;

        // === UAD-style 3D rocker-switch (Neve 8028 Bypass-Switch / Impedance-Switch-pattern) ===

        // 1. Drop-shadow under switch
        for (int i = 0; i < 2; ++i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.30f - i * 0.12f));
            g.fillRoundedRectangle (bounds.translated (0, 1.0f + i), radius);
        }

        // 2. Switch-rocker-body (3D-skuggat: ENGAGED = lower position, OFF = raised)
        const float pressOffset = engaged ? 1.0f : 0.0f;
        auto rocker = bounds.translated (0, pressOffset);

        // Vertical gradient: chrome top → dark bottom (när ej engaged)
        // ENGAGED = inverterad gradient (visar att switchen är "neddtryckt")
        const juce::Colour topCol    = engaged ? juce::Colour (0xff4a4842) : juce::Colour (0xffe8e5db);
        const juce::Colour midCol    = engaged ? juce::Colour (0xff6a6862) : juce::Colour (0xffc8c5bb);
        const juce::Colour bottomCol = engaged ? juce::Colour (0xff8a8780) : juce::Colour (0xff7a7872);

        juce::ColourGradient body (
            topCol,    rocker.getX(), rocker.getY(),
            bottomCol, rocker.getX(), rocker.getBottom(), false);
        body.addColour (0.5, midCol);
        g.setGradientFill (body);
        g.fillRoundedRectangle (rocker, radius);

        // 3. Engaged-accent: röd LED-strip överst när engaged (UAD-pattern: state-LED)
        if (engaged)
        {
            const auto led = juce::Rectangle<float> (
                rocker.getX() + 3, rocker.getY() + 2, rocker.getWidth() - 6, 2.0f);
            juce::ColourGradient ledGrad (
                juce::Colour (0xffe04020), led.getX(), led.getY(),
                juce::Colour (0xff6a1810), led.getX(), led.getBottom(), false);
            g.setGradientFill (ledGrad);
            g.fillRoundedRectangle (led, 1.0f);
        }

        // 4. Top specular-highlight (chrome-shine)
        if (! engaged)
        {
            juce::ColourGradient sheen (
                juce::Colours::white.withAlpha (0.55f), rocker.getX(), rocker.getY(),
                juce::Colours::white.withAlpha (0.0f), rocker.getX(), rocker.getY() + rocker.getHeight() * 0.45f, false);
            g.setGradientFill (sheen);
            g.fillRoundedRectangle (
                rocker.withTrimmedBottom (rocker.getHeight() * 0.55f).reduced (1.5f, 1.0f), radius);
        }

        // 5. Yttre svart rim (skarp kant)
        g.setColour (juce::Colour (0xff0a0806));
        g.drawRoundedRectangle (rocker, radius, 0.7f);

        // 6. Hover-glow
        if (isHighlighted && ! isDown)
        {
            g.setColour (juce::Colour (0xffe88030).withAlpha (0.20f));
            g.drawRoundedRectangle (rocker.expanded (1.5f), radius + 1.0f, 1.5f);
        }
    }

    void BCLookAndFeel::drawButtonText (juce::Graphics& g,
                                        juce::TextButton& button,
                                        bool isHighlighted,
                                        bool isDown)
    {
        juce::ignoreUnused (isHighlighted);
        const bool engaged = button.getToggleState() || isDown;
        const auto bounds = button.getLocalBounds().toFloat()
            .translated (0, engaged ? 1.0f : 0.0f);  // press-offset

        // Engagerad = vit text på mörk rocker, off = svart text på chrome
        g.setColour (engaged ? juce::Colour (0xfff5f2ea) : juce::Colour (0xff141210));
        g.setFont (juce::Font (juce::FontOptions (8.5f).withName ("Helvetica").withStyle ("Bold")));
        g.drawText (button.getButtonText(), bounds.toNearestInt(),
                    juce::Justification::centred, false);
    }

    void BCLookAndFeel::drawComboBox (juce::Graphics& g,
                                      int width, int height,
                                      bool isButtonDown,
                                      int buttonX, int buttonY, int buttonW, int buttonH,
                                      juce::ComboBox& box)
    {
        juce::ignoreUnused (isButtonDown, buttonX, buttonY, buttonW, buttonH, box);

        const auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                                     (float) width, (float) height).reduced (1.0f);

        // Mörk dark-recess bakgrund
        juce::ColourGradient bg (kRecessTop, bounds.getX(), bounds.getY(),
                                  kRecessBot, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (bg);
        g.fillRoundedRectangle (bounds, 2.0f);

        // Border (svart)
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (bounds, 2.0f, 0.7f);

        // Subtila inner-skugga
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawLine (bounds.getX() + 2, bounds.getY() + 1.5f,
                    bounds.getRight() - 2, bounds.getY() + 1.5f, 1.0f);

        // Pil
        const float arrowSize = 5.0f;
        const float arrowX = bounds.getRight() - arrowSize - 6.0f;
        const float arrowY = bounds.getCentreY();
        juce::Path arrow;
        arrow.addTriangle (arrowX, arrowY - arrowSize * 0.4f,
                           arrowX + arrowSize, arrowY - arrowSize * 0.4f,
                           arrowX + arrowSize * 0.5f, arrowY + arrowSize * 0.4f);
        g.setColour (kAmber);
        g.fillPath (arrow);
    }
}
