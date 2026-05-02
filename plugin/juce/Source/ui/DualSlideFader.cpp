/*  DualSlideFader implementation. */

#include "DualSlideFader.h"

namespace bc2000dl::ui
{
    using namespace bc2000dl::ui::colours;

    void DualSlideFader::DualLook::drawLinearSlider (juce::Graphics& g,
                                                     int x, int y, int w, int h,
                                                     float sliderPos,
                                                     float minPos, float maxPos,
                                                     juce::Slider::SliderStyle style,
                                                     juce::Slider& slider)
    {
        juce::ignoreUnused (minPos, maxPos, style, slider);

        const auto bounds = juce::Rectangle<float> (
            (float) x, (float) y, (float) w, (float) h);

        // === UAD-style channel-strip thumb (bred + prominent center engagement line) ===
        const float knobW = (float) w * 1.35f;     // bredare (UAD ref-image stil)
        const float knobH = 32.0f;                  // högre
        const float knobY = sliderPos - knobH * 0.5f;
        const float knobX = bounds.getCentreX() - knobW * 0.5f;
        const float radius = 3.0f;

        // 1. Drop-shadow (mjuk, ger djup)
        for (int i = 0; i < 3; ++i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.18f - i * 0.05f));
            g.fillRoundedRectangle (knobX - i * 0.5f, knobY + 1.5f + i,
                                     knobW + i, knobH + i, radius);
        }

        // 2. Chrome body — radial gradient ger 3D-känsla (inte bara linjär)
        juce::ColourGradient body (
            juce::Colour (0xfff5f2ea),
            knobX + knobW * 0.5f, knobY + knobH * 0.35f,
            juce::Colour (0xff5e5b54),
            knobX + knobW * 0.5f, knobY + knobH * 1.3f, true);
        body.addColour (0.35, juce::Colour (0xffe8e5dc));
        body.addColour (0.55, juce::Colour (0xffc0bdb3));
        body.addColour (0.75, juce::Colour (0xff8a8780));
        g.setGradientFill (body);
        g.fillRoundedRectangle (knobX, knobY, knobW, knobH, radius);

        // 3. Top specular-highlight (smal ljusstreck längst upp för chrome-look)
        juce::ColourGradient topHl (
            juce::Colours::white.withAlpha (0.85f),
            knobX, knobY + 1,
            juce::Colours::white.withAlpha (0.0f),
            knobX, knobY + knobH * 0.35f, false);
        g.setGradientFill (topHl);
        g.fillRoundedRectangle (knobX + 1, knobY + 1, knobW - 2, knobH * 0.4f, radius);

        // 4. Bottom dark-rim (skugga längst ner)
        juce::ColourGradient botShade (
            juce::Colour (0xff353331).withAlpha (0.0f),
            knobX, knobY + knobH * 0.65f,
            juce::Colour (0xff1a1814).withAlpha (0.7f),
            knobX, knobY + knobH, false);
        g.setGradientFill (botShade);
        g.fillRoundedRectangle (knobX + 1, knobY + knobH * 0.6f,
                                 knobW - 2, knobH * 0.4f, radius);

        // 5. Outer-rim (skarp svart kant)
        g.setColour (juce::Colour (0xff0c0a08));
        g.drawRoundedRectangle (knobX + 0.5f, knobY + 0.5f, knobW - 1, knobH - 1, radius, 0.8f);

        // 6. 6 grip-räfflor (smalare + tätare = mer realistiskt)
        const float gripStartX = knobX + knobW * 0.18f;
        const float gripEndX   = knobX + knobW * 0.82f;
        const int nGrips = 6;
        for (int i = 0; i < nGrips; ++i)
        {
            const float gx = gripStartX + (gripEndX - gripStartX) * i / (float) (nGrips - 1);
            // Djup mörkt streck
            g.setColour (juce::Colour (0xff0a0806).withAlpha (0.75f));
            g.fillRect (gx - 0.6f, knobY + 4.0f, 1.2f, knobH - 8.0f);
            // Ljus highlight bredvid (vit kant ger metallisk räffla)
            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.fillRect (gx + 0.6f, knobY + 4.0f, 0.6f, knobH - 8.0f);
        }

        // 7. UAD-style PROMINENT engagement-line genom mitten
        // Tjock svart linje med subtil 3D-effekt — visar precist värde-position
        const float lineY = knobY + knobH * 0.5f;
        // Skugga under linjen
        g.setColour (juce::Colour (0xff0a0806).withAlpha (0.6f));
        g.fillRect (knobX + 2, lineY - 1.0f, knobW - 4, 2.5f);
        // Mörk huvudlinje
        g.setColour (juce::Colour (0xff050403));
        g.fillRect (knobX + 3, lineY - 0.5f, knobW - 6, 1.5f);
        // Tunn vit highlight ovanpå (chrome-cap)
        g.setColour (juce::Colour (0xfff5f2ea).withAlpha (0.5f));
        g.fillRect (knobX + 5, lineY - 1.0f, knobW - 10, 0.4f);

        // 8. L/R-indikator (subtilare nu — mindre bold)
        g.setColour (juce::Colour (0xff141210).withAlpha (0.65f));
        g.setFont (juce::Font (juce::FontOptions (6.5f).withName ("Helvetica")).boldened());
        const float indX = isLeftKnob ? knobX - 10.0f : knobX + knobW + 4.0f;
        g.drawText (isLeftKnob ? "L" : "R",
                    juce::Rectangle<float> (indX, knobY + 6.0f, 6.0f, 10.0f),
                    juce::Justification::centred);
    }

    DualSlideFader::DualSlideFader (const juce::String& sym)
        : symbol (sym)
    {
        sliderL.setSliderStyle (juce::Slider::LinearVertical);
        sliderL.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        sliderL.setLookAndFeel (&lookL);
        addAndMakeVisible (sliderL);

        sliderR.setSliderStyle (juce::Slider::LinearVertical);
        sliderR.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        sliderR.setLookAndFeel (&lookR);
        addAndMakeVisible (sliderR);

        configurePremiumSliderUX (sliderL);
        configurePremiumSliderUX (sliderR);

        symbolLabel.setText (symbol, juce::dontSendNotification);
        symbolLabel.setJustificationType (juce::Justification::centred);
        symbolLabel.setColour (juce::Label::textColourId, kSilverLight);
        symbolLabel.setFont (juce::Font (juce::FontOptions (10.0f).withName ("Helvetica")).boldened());
        addAndMakeVisible (symbolLabel);
    }

    void DualSlideFader::configurePremiumSliderUX (juce::Slider& s)
    {
        // UAD-pattern UX:
        //  • Wheel scroll = adjust value
        //  • Double-click = reset (sätts via setDefaultValue)
        //  • Right-click = snap menu (custom mouseDown i PremiumSlider)
        //  • Cmd-drag = fine mode (custom mouseDrag i PremiumSlider)
        s.setScrollWheelEnabled (true);
        s.setMouseDragSensitivity (200);
        s.setVelocityBasedMode (false);
    }

    void DualSlideFader::PremiumSlider::mouseDown (const juce::MouseEvent& e)
    {
        if (e.mods.isPopupMenu())
        {
            owner.showSnapMenu (*this);
            return;
        }
        juce::Slider::mouseDown (e);
    }

    void DualSlideFader::PremiumSlider::mouseDrag (const juce::MouseEvent& e)
    {
        // Cmd-drag = fine mode (10× långsammare via en simulerad event)
        if (e.mods.isCommandDown())
        {
            // Skapa modifierad event där distansen reduceras 10×
            auto fine = e.withNewPosition (e.getMouseDownPosition()
                + (e.getPosition() - e.getMouseDownPosition()) / 10);
            juce::Slider::mouseDrag (fine);
            return;
        }
        juce::Slider::mouseDrag (e);
    }

    void DualSlideFader::setDefaultValue (double v)
    {
        defaultVal = v;
        sliderL.setDoubleClickReturnValue (true, v);
        sliderR.setDoubleClickReturnValue (true, v);
        repaint();
    }

    void DualSlideFader::setSnapPoints (const juce::Array<double>& points)
    {
        snapPoints = points;
    }

    void DualSlideFader::showSnapMenu (juce::Slider& s)
    {
        juce::PopupMenu m;
        m.addItem (1, "Reset to default", true);
        m.addItem (2, "Set value…", true);
        m.addSeparator();

        if (snapPoints.isEmpty())
        {
            // Defaultsnap-punkter om inget annat satts
            const double r = s.getMaximum() - s.getMinimum();
            m.addItem (10, "Min ("  + juce::String (s.getMinimum(), 1) + ")",  true);
            m.addItem (11, "25 % (" + juce::String (s.getMinimum() + r * 0.25, 1) + ")", true);
            m.addItem (12, "Mid (" + juce::String (s.getMinimum() + r * 0.5, 1)  + ")", true);
            m.addItem (13, "75 % (" + juce::String (s.getMinimum() + r * 0.75, 1) + ")", true);
            m.addItem (14, "Max (" + juce::String (s.getMaximum(), 1) + ")",  true);
        }
        else
        {
            int id = 100;
            for (auto pt : snapPoints)
            {
                const auto label = juce::String (pt == -INFINITY ? "-∞" : juce::String (pt, 1));
                m.addItem (id++, label, true);
            }
        }

        m.showMenuAsync (juce::PopupMenu::Options{}, [this, &s] (int chosen)
        {
            if (chosen == 0) return;
            if (chosen == 1) { s.setValue (defaultVal); return; }
            if (chosen == 2) { s.showTextBox(); return; }
            if (chosen >= 10 && chosen <= 14)
            {
                const double r = s.getMaximum() - s.getMinimum();
                const double v = (chosen == 10) ? s.getMinimum()
                                : (chosen == 11) ? s.getMinimum() + r * 0.25
                                : (chosen == 12) ? s.getMinimum() + r * 0.5
                                : (chosen == 13) ? s.getMinimum() + r * 0.75
                                                 : s.getMaximum();
                s.setValue (v);
                return;
            }
            if (chosen >= 100 && chosen < 100 + snapPoints.size())
            {
                const double pt = snapPoints[chosen - 100];
                if (pt != -INFINITY) s.setValue (pt);
                else s.setValue (s.getMinimum());
            }
        });
    }

    void DualSlideFader::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();

        // === Noisehead-style djupare slot-track ===
        const float trackW = 6.0f;
        const float trackX = bounds.getCentreX() - trackW * 0.5f;
        const float trackTop = bounds.getY() + 18.0f;
        const float trackBottom = bounds.getBottom() - 8.0f;
        const float trackH = trackBottom - trackTop;
        const float radius = 2.5f;

        // 1. Yttre skugga (ger inset-djup-känsla)
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (trackX - 1, trackTop - 1, trackW + 2, trackH + 2, radius);

        // 2. Mörk slot-bakgrund (svart fördjupning i panelen)
        juce::ColourGradient slotGrad (
            juce::Colour (0xff050403), trackX, trackTop,
            juce::Colour (0xff1a1714), trackX, trackBottom, false);
        g.setGradientFill (slotGrad);
        g.fillRoundedRectangle (trackX, trackTop, trackW, trackH, radius);

        // 3. Inner top-shadow (visar att det är insänkt)
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.drawLine (trackX + 0.5f, trackTop + 0.5f, trackX + trackW - 0.5f, trackTop + 0.5f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawLine (trackX + 0.5f, trackTop + 1.5f, trackX + trackW - 0.5f, trackTop + 1.5f, 0.7f);

        // 4. Inner bottom-highlight (subtil reflektion från botten)
        g.setColour (juce::Colour (0xff5a5852).withAlpha (0.30f));
        g.drawLine (trackX + 0.5f, trackBottom - 0.5f, trackX + trackW - 0.5f, trackBottom - 0.5f, 0.7f);

        // === BC2000 0-10 skala på vänster sida ===
        g.setColour (kSilverLight);
        g.setFont (juce::Font (juce::FontOptions (7.0f).withName ("Helvetica")));
        for (int i = 0; i <= 10; ++i)
        {
            const float ty = trackTop + trackH * (i / 10.0f);
            const int displayNum = 10 - i;
            const bool isMajor = (i % 5 == 0);

            // Streck-marker (länger för 0/5/10)
            g.setColour (isMajor ? kSilverLight : kSoftGray);
            g.fillRect (trackX - (isMajor ? 16.0f : 12.0f), ty - 0.5f,
                        isMajor ? 5.0f : 3.0f, 1.0f);

            // Number-label
            g.setColour (kSilverLight);
            g.drawText (juce::String (displayNum),
                        juce::Rectangle<float> (trackX - 28.0f, ty - 5.0f, 10.0f, 10.0f),
                        juce::Justification::centredRight);
        }

        // === Reference mark vid default-värde (UAD-pattern) — NU TYDLIGARE ===
        const auto rangeMin = sliderL.getMinimum();
        const auto rangeMax = sliderL.getMaximum();
        if (rangeMax > rangeMin)
        {
            const double t = juce::jlimit (0.0, 1.0,
                (defaultVal - rangeMin) / (rangeMax - rangeMin));
            const float refY = trackBottom - (float) t * trackH;
            // Större vit triangel-pekare på höger sida
            juce::Path tri;
            tri.addTriangle (trackX + trackW + 5.0f, refY,
                             trackX + trackW + 14.0f, refY - 5.0f,
                             trackX + trackW + 14.0f, refY + 5.0f);
            g.setColour (juce::Colour (0xfff5f2ea));
            g.fillPath (tri);
            g.setColour (juce::Colour (0xff050403));
            g.strokePath (tri, juce::PathStrokeType (0.7f));
        }

        // === UAD-style numerisk dB-display under faders (visar L+R medel-värde) ===
        const double valL = sliderL.getValue();
        const double valR = sliderR.getValue();
        const double avg = (valL + valR) * 0.5;
        // Map 0..1 → -∞..+12 dB (UAD-fader-curve)
        const double normalized = (avg - rangeMin) / std::max (rangeMax - rangeMin, 1e-9);
        const double dbValue = normalized > 0.001
            ? 20.0 * std::log10 (normalized * 4.0)              // 0=−∞, 1=+12
            : -INFINITY;

        const juce::String dbStr = (dbValue == -INFINITY)
            ? juce::String ("-∞")
            : (dbValue >= 0 ? "+" + juce::String (dbValue, 1) : juce::String (dbValue, 1));

        // Mörk dB-display-bakgrund nere
        const auto dispR = juce::Rectangle<float> (
            bounds.getX(), bounds.getBottom() - 14.0f,
            bounds.getWidth(), 12.0f);
        g.setColour (juce::Colour (0xff0a0806).withAlpha (0.8f));
        g.fillRoundedRectangle (dispR, 2.0f);
        g.setColour (juce::Colour (0xff5a5852));
        g.drawRoundedRectangle (dispR, 2.0f, 0.5f);

        // Amber-numerisk text (UAD-style LCD-look)
        g.setColour (juce::Colour (0xffe8a040));
        g.setFont (juce::Font (juce::FontOptions (8.5f).withName ("Helvetica").withStyle ("Bold")));
        g.drawText (dbStr, dispR, juce::Justification::centred, false);
    }

    void DualSlideFader::resized()
    {
        auto bounds = getLocalBounds();

        // Symbol ÖVERST (matchar B&O-original — symboler står ovanför fadrarna)
        symbolLabel.setBounds (bounds.removeFromTop (16));

        // Två slidrar bredvid varandra, var och en tar halva bredden
        const auto leftHalf  = bounds.removeFromLeft (bounds.getWidth() / 2);
        const auto rightHalf = bounds;

        sliderL.setBounds (leftHalf);
        sliderR.setBounds (rightHalf);
    }
}
