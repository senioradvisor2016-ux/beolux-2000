/*  BC2000LookAndFeel implementation — hardware-accurate Beocord 2000 DL aesthetic. */

#include "BC2000LookAndFeel.h"

namespace bc2000dl::ui
{
    //=========================================================================
    //  Construction
    //=========================================================================
    InstructionCardLnF::InstructionCardLnF()
    {
        // CREAM PAPER instruction-card defaults — controls live on warm
        // cream paper, text in dark ink.
        const auto inkCol  = juce::Colour (0xFF181408);
        const auto creamUI = juce::Colour (0xFFE5DBC0);

        setColour (juce::ResizableWindow::backgroundColourId, creamUI);
        setColour (juce::DocumentWindow::backgroundColourId, creamUI);
        setColour (juce::Label::textColourId, inkCol);

        setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xFFF0E6CC));
        setColour (juce::TextEditor::textColourId, inkCol);
        setColour (juce::TextEditor::outlineColourId, inkCol.withAlpha (0.6f));

        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFFF0E6CC));
        setColour (juce::ComboBox::textColourId, inkCol);
        setColour (juce::ComboBox::outlineColourId, inkCol.withAlpha (0.7f));
        setColour (juce::ComboBox::arrowColourId, inkCol);

        // Popup menu stays dark for legibility against bright cream backdrop
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xFF181408));
        setColour (juce::PopupMenu::textColourId, paper());
        setColour (juce::PopupMenu::highlightedBackgroundColourId, redAccent());
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        setColour (juce::TextButton::buttonColourId, juce::Colour (0xFFE5DBC0));
        setColour (juce::TextButton::textColourOffId, inkCol);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);

        setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xFF181408));
        setColour (juce::TooltipWindow::textColourId, paper());
        setColour (juce::TooltipWindow::outlineColourId, inkCol.withAlpha (0.6f));
    }

    //=========================================================================
    //  Typography
    //=========================================================================
    juce::Font InstructionCardLnF::labelFont (float sizePx)
    {
        juce::FontOptions o ("Helvetica Neue", sizePx, juce::Font::plain);
        return juce::Font (o.withStyle ("Condensed Bold"));
    }

    juce::Font InstructionCardLnF::sectionFont (float sizePx)
    {
        juce::FontOptions o ("Helvetica Neue", sizePx, juce::Font::bold);
        auto f = juce::Font (o.withStyle ("Condensed Bold"));
        f.setExtraKerningFactor (0.08f);
        return f;
    }

    juce::Font InstructionCardLnF::monoFont (float sizePx)
    {
        juce::FontOptions o ("Menlo", sizePx, juce::Font::bold);
        return juce::Font (o);
    }

    juce::Font InstructionCardLnF::logoFont (float sizePx)
    {
        juce::FontOptions o ("Helvetica Neue", sizePx, juce::Font::bold);
        return juce::Font (o);
    }

    juce::Font InstructionCardLnF::getComboBoxFont   (juce::ComboBox&)              { return labelFont (11.0f); }
    juce::Font InstructionCardLnF::getLabelFont      (juce::Label&)                 { return labelFont (11.0f); }
    juce::Font InstructionCardLnF::getTextButtonFont (juce::TextButton&, int h)     { return labelFont ((float) juce::jmin (12, h - 6)); }
    juce::Font InstructionCardLnF::getPopupMenuFont  ()                             { return labelFont (12.0f); }

    void InstructionCardLnF::getIdealPopupMenuItemSize (const juce::String& text,
                                                         bool isSeparator,
                                                         int /*standard*/,
                                                         int& w, int& h)
    {
        if (isSeparator) { w = 50; h = 6; return; }
        auto f = getPopupMenuFont();
        w = juce::jmax (110, juce::GlyphArrangement::getStringWidthInt (f, text) + 36);
        h = 22;
    }

    void InstructionCardLnF::drawPopupMenuItem (juce::Graphics& g,
                                                 const juce::Rectangle<int>& area,
                                                 bool isSeparator, bool isActive,
                                                 bool isHighlighted, bool isTicked,
                                                 bool /*hasSubMenu*/,
                                                 const juce::String& text,
                                                 const juce::String& /*shortcutKeyText*/,
                                                 const juce::Drawable* /*icon*/,
                                                 const juce::Colour* textColour)
    {
        if (isSeparator)
        {
            auto r = area.toFloat().reduced (4.0f, 0.0f);
            g.setColour (panelEdge());
            g.fillRect (r.withSizeKeepingCentre (r.getWidth(), 1.0f));
            return;
        }

        if (isHighlighted && isActive)
        {
            g.setColour (redAccent());
            g.fillRect (area);
            g.setColour (paper());
        }
        else
        {
            g.setColour (textColour != nullptr ? *textColour : creamLabel());
        }

        auto r = area.reduced (10, 0);

        if (isTicked)
        {
            const auto tick = r.removeFromLeft (12);
            juce::Path p;
            const float cx = (float) tick.getCentreX();
            const float cy = (float) tick.getCentreY();
            p.startNewSubPath (cx - 5.0f, cy);
            p.lineTo (cx - 1.0f, cy + 4.0f);
            p.lineTo (cx + 5.0f, cy - 4.0f);
            g.strokePath (p, juce::PathStrokeType (1.6f));
        }
        else
        {
            r.removeFromLeft (12);
        }

        g.setFont (getPopupMenuFont());
        g.drawFittedText (text, r, juce::Justification::centredLeft, 1);
    }

    //=========================================================================
    //  Slider rendering — vertical slide-faders
    //=========================================================================
    void InstructionCardLnF::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                                                float sliderPos, float, float,
                                                juce::Slider::SliderStyle style,
                                                juce::Slider& s)
    {
        if (style != juce::Slider::LinearVertical)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, 0, 0, style, s);
            return;
        }

        const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        // BIG cap so the user can't miss the drag target — the real Beocord
        // had chunky chrome thumbs you could grab without looking.
        const float capH = juce::jlimit (24.0f, 36.0f, area.getHeight() * 0.18f);
        const float capW = juce::jmin ((float) w - 2.0f, 30.0f);
        const float channelW = 5.0f;

        // Channel (recessed slot with white-painted scale on each side)
        const float chTop = area.getY() + capH * 0.5f;
        const float chBot = area.getBottom() - capH * 0.5f;
        const juce::Rectangle<float> channel (
            area.getCentreX() - channelW * 0.5f, chTop, channelW, chBot - chTop);
        drawSlideChannel (g, channel);

        // Cap with hover/drag highlight
        const float capY = sliderPos - capH * 0.5f;
        const juce::Rectangle<float> cap (area.getCentreX() - capW * 0.5f, capY, capW, capH);
        const bool hot = s.isMouseOverOrDragging();
        drawSlideFaderCap (g, cap, hot);
    }

    void InstructionCardLnF::drawSlideChannel (juce::Graphics& g, juce::Rectangle<float> ch)
    {
        // ---- Channel: a thin black-ink slot on cream paper ----
        // The Beocord instruction card draws a dotted vertical line for each
        // slide path. We use a solid recessed slot with cream interior so the
        // chrome cap reads against a contrasting tone.
        g.setColour (juce::Colour (0xFFCFC4A6));        // recessed paper-tone
        g.fillRoundedRectangle (ch, 1.2f);
        g.setColour (juce::Colour (0xFF181408).withAlpha (0.8f));
        g.drawRoundedRectangle (ch, 1.2f, 0.7f);

        // Dotted centre track (the iconic instruction-card look)
        const float midX = ch.getCentreX();
        g.setColour (juce::Colour (0xFF181408).withAlpha (0.45f));
        for (float dy = ch.getY() + 2.0f; dy < ch.getBottom() - 1.0f; dy += 3.0f)
            g.fillRect (midX - 0.5f, dy, 1.0f, 1.5f);

        // Tick marks (printed as small black dashes either side, longer at 0/50/100)
        g.setColour (juce::Colour (0xFF181408).withAlpha (0.85f));
        for (int i = 0; i <= 10; ++i)
        {
            const float ty = ch.getY() + (ch.getHeight() * (float) i / 10.0f);
            const float tw = (i == 0 || i == 5 || i == 10) ? 8.0f : 5.0f;
            g.drawLine (ch.getX() - tw - 1.0f, ty, ch.getX() - 2.0f, ty, 0.85f);
            g.drawLine (ch.getRight() + 2.0f, ty, ch.getRight() + tw + 1.0f, ty, 0.85f);
        }
    }

    void InstructionCardLnF::drawSlideFaderCap (juce::Graphics& g, juce::Rectangle<float> r,
                                                  bool bright)
    {
        // AUTENTIC MATTE ALUMINIUM fader cap — anodised silver-grey, satin
        // (no chrome highlights). Subtle top edge, red position-marker line.
        // The real Beocord cap is a small extruded aluminium block with a
        // recessed red stripe across the centre.

        // ---- Soft drop shadow ----
        g.setColour (juce::Colours::black.withAlpha (0.40f));
        g.fillRoundedRectangle (r.translated (0, 1.0f).expanded (0.3f, 0.3f), 2.5f);

        // ---- Matte aluminium body (very subtle vertical gradient) ----
        juce::ColourGradient grad (
            juce::Colour (0xFFB8B8B2), r.getCentreX(), r.getY(),
            juce::Colour (0xFF8E8E88), r.getCentreX(), r.getBottom(), false);
        grad.addColour (0.50, juce::Colour (0xFFA0A09A));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 2.5f);

        // ---- Faint horizontal brushed-anodise grain (stable seeded noise) ----
        juce::Random rng (int (r.getY() * 17.0f) ^ int (r.getX() * 31.0f));
        for (int i = 0; i < 18; ++i)
        {
            const float fy = r.getY() + 1.5f + rng.nextFloat() * (r.getHeight() - 3.0f);
            const bool dark = rng.nextBool();
            g.setColour ((dark ? juce::Colour (0xFF6E6E68) : juce::Colour (0xFFD2D2CC))
                            .withAlpha (rng.nextFloat() * 0.10f + 0.02f));
            g.drawLine (r.getX() + 2.0f, fy, r.getRight() - 2.0f, fy, 0.4f);
        }

        // ---- Top hairline highlight ----
        g.setColour (juce::Colour (0xFFE0E0DA).withAlpha (0.55f));
        g.drawLine (r.getX() + 2.0f, r.getY() + 0.6f,
                    r.getRight() - 2.0f, r.getY() + 0.6f, 0.6f);

        // ---- Bottom edge shadow ----
        g.setColour (juce::Colour (0xFF202020).withAlpha (0.35f));
        g.drawLine (r.getX() + 2.0f, r.getBottom() - 0.6f,
                    r.getRight() - 2.0f, r.getBottom() - 0.6f, 0.6f);

        // ---- Crisp dark outline ----
        g.setColour (juce::Colour (0xFF252522).withAlpha (0.85f));
        g.drawRoundedRectangle (r, 2.5f, 0.8f);

        // ---- Recessed red position-marker line ----
        const float cy = r.getCentreY();
        const float barH = 2.0f;
        const auto bar = juce::Rectangle<float> (r.getX() + 2.5f, cy - barH * 0.5f,
                                                  r.getWidth() - 5.0f, barH);
        // Recess shadow above the bar
        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.drawLine (bar.getX(), bar.getY() - 0.5f, bar.getRight(), bar.getY() - 0.5f, 0.5f);
        // Red paint inside the recess
        juce::ColourGradient barGrad (
            juce::Colour (0xFFD23828), bar.getCentreX(), bar.getY(),
            juce::Colour (0xFF8C1810), bar.getCentreX(), bar.getBottom(), false);
        g.setGradientFill (barGrad);
        g.fillRect (bar);

        // ---- Hot amber ring on hover/drag ----
        if (bright)
        {
            g.setColour (juce::Colour (0xFFC2A050).withAlpha (0.35f));
            g.drawRoundedRectangle (r.expanded (0.8f), 3.0f, 1.0f);
        }
    }

    //=========================================================================
    //  Rotary slider — chrome ring, dark body, amber indicator
    //=========================================================================
    void InstructionCardLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                                float sliderPos,
                                                float angleStart, float angleEnd,
                                                juce::Slider& s)
    {
        // INSTRUCTION-CARD STYLE: thin black-ink circle on cream paper, with a
        // curved "rotation-direction" arrow at the top — exactly the symbol
        // language used on the original B&O Beocord 2000 DL operating-card.
        const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h)
                              .reduced (4.0f);
        const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
        const float cx = area.getCentreX();
        const float cy = area.getCentreY();
        const float angle = angleStart + sliderPos * (angleEnd - angleStart);
        const bool hot = s.isMouseOverOrDragging();

        const auto inkCol = juce::Colour (0xFF181408);

        // ---- Hover halo (warm amber on cream) ----
        if (hot)
        {
            g.setColour (juce::Colour (0xFFC2A050).withAlpha (0.20f));
            g.fillEllipse (cx - radius - 2, cy - radius - 2,
                           (radius + 2) * 2, (radius + 2) * 2);
        }

        // ---- Outer ring (thin black ink line — like a printed schematic) ----
        const float bezR = radius * 0.95f;
        g.setColour (inkCol.withAlpha (0.85f));
        g.drawEllipse (cx - bezR, cy - bezR, bezR * 2, bezR * 2, 1.4f);

        // ---- Tick marks at min/max ----
        for (float a : { angleStart, angleEnd })
        {
            const float r1 = bezR + 1.5f;
            const float r2 = bezR + 5.0f;
            g.drawLine (cx + r1 * std::sin (a), cy - r1 * std::cos (a),
                        cx + r2 * std::sin (a), cy - r2 * std::cos (a), 1.0f);
        }

        // ---- Knob body: dark satin (matte black plastic, like real B&O knobs) ----
        const float bodyR = radius * 0.82f;
        juce::ColourGradient bodyGrad (
            juce::Colour (0xFF3A3A3E), cx, cy - bodyR,
            juce::Colour (0xFF0E0E10), cx, cy + bodyR, false);
        bodyGrad.addColour (0.45, juce::Colour (0xFF202024));
        g.setGradientFill (bodyGrad);
        g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2, bodyR * 2);

        // Body rim (thin chrome ring at outer edge)
        g.setColour (juce::Colour (0xFF8A8A88));
        g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2, bodyR * 2, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawEllipse (cx - bodyR + 0.7f, cy - bodyR + 0.7f, (bodyR - 0.7f) * 2, (bodyR - 0.7f) * 2, 0.5f);

        // ---- Pointer: white indicator line (the iconic B&O white dot/line) ----
        const float indR0 = bodyR * 0.20f;
        const float indR1 = bodyR * 0.95f;
        const float sx = cx + indR0 * std::sin (angle);
        const float sy = cy - indR0 * std::cos (angle);
        const float ex = cx + indR1 * std::sin (angle);
        const float ey = cy - indR1 * std::cos (angle);
        // White inner line
        g.setColour (juce::Colours::white.withAlpha (0.95f));
        g.drawLine (sx, sy, ex, ey, 2.4f);
        // Red tip dot
        g.setColour (s.isEnabled() ? redAccent() : creamDim());
        g.fillEllipse (ex - 2.2f, ey - 2.2f, 4.4f, 4.4f);

        // ---- Centre dot (small white) ----
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.fillEllipse (cx - 1.4f, cy - 1.4f, 2.8f, 2.8f);

        // ---- Curved arrow above the knob (printed-schematic indicator) ----
        // The Beocord card always shows a curly arrow on either side of bigger
        // tone knobs to indicate direction-of-increase.
        const float arrR = bezR + 8.0f;
        const float a0 = -juce::MathConstants<float>::pi * 0.55f;
        const float a1 = -juce::MathConstants<float>::pi * 0.20f;
        juce::Path arrow;
        arrow.addCentredArc (cx, cy, arrR, arrR, 0, a0, a1, true);
        g.setColour (inkCol.withAlpha (0.7f));
        g.strokePath (arrow, juce::PathStrokeType (1.0f));
        // Arrowhead at end of arc
        const float ahx = cx + arrR * std::sin (a1);
        const float ahy = cy - arrR * std::cos (a1);
        juce::Path head;
        head.startNewSubPath (ahx, ahy);
        head.lineTo (ahx - 4.0f, ahy - 1.5f);
        head.lineTo (ahx - 2.5f, ahy + 3.5f);
        head.closeSubPath();
        g.fillPath (head);

        // ---- Hot value arc (amber, on top of ring when interacting) ----
        if (hot)
        {
            const float trackR = bezR + 3.0f;
            juce::Path arcFill;
            arcFill.addCentredArc (cx, cy, trackR, trackR, 0, angleStart, angle, true);
            g.setColour (juce::Colour (0xFFC2A050));
            g.strokePath (arcFill, juce::PathStrokeType (1.6f));
        }
    }

    //=========================================================================
    //  Toggle button — dark panel, red when ON
    //=========================================================================
    void InstructionCardLnF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                                 bool highlighted, bool down)
    {
        // INSTRUCTION-CARD STYLE: thin black-ink outlined rectangle on cream
        // paper. Filled with red when ON. Crisp, schematic, exactly like the
        // pictogram boxes printed on the original B&O card.
        const auto bounds = b.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = b.getToggleState();
        const auto inkCol = juce::Colour (0xFF181408);

        // ---- Body fill ----
        if (on)
        {
            // Red filled box when ON (like the red "REC" indicator on card)
            juce::ColourGradient g1 (
                juce::Colour (0xFFD4382A), bounds.getCentreX(), bounds.getY(),
                juce::Colour (0xFFA22018), bounds.getCentreX(), bounds.getBottom(), false);
            g.setGradientFill (g1);
            g.fillRoundedRectangle (bounds, 2.5f);
        }
        else if (down || highlighted)
        {
            g.setColour (juce::Colour (0xFFD9CFB2));
            g.fillRoundedRectangle (bounds, 2.5f);
        }
        else
        {
            // Slight tinted recess
            g.setColour (juce::Colour (0xFFE5DBC0));
            g.fillRoundedRectangle (bounds, 2.5f);
        }

        // ---- Black-ink outline ----
        g.setColour (inkCol.withAlpha (on ? 0.7f : 0.85f));
        g.drawRoundedRectangle (bounds, 2.5f, 1.0f);

        // ---- Label centred (white on red, dark ink on cream) ----
        g.setColour (on ? juce::Colours::white : inkCol);
        g.setFont (sectionFont (9.5f));
        g.drawFittedText (b.getButtonText(), bounds.reduced (4, 1).toNearestInt(),
                           juce::Justification::centred, 1);
    }

    //=========================================================================
    //  Button background — transport keys (dark hardware style)
    //=========================================================================
    void InstructionCardLnF::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                                     const juce::Colour& /*bgColour*/,
                                                     bool highlighted, bool down)
    {
        drawTransportKey (g, b.getLocalBounds(), down, b.getToggleState() || down,
                           b.getName().contains ("REC") ? redAccent() : amber());
        juce::ignoreUnused (highlighted);
    }

    //=========================================================================
    //  Combo box — dark with amber arrow
    //=========================================================================
    void InstructionCardLnF::drawComboBox (juce::Graphics& g, int width, int height,
                                            bool /*isButtonDown*/,
                                            int buttonX, int buttonY,
                                            int buttonW, int buttonH,
                                            juce::ComboBox& cb)
    {
        const auto box = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
        const auto inkCol = juce::Colour (0xFF181408);

        // Cream-paper recessed-look fill
        juce::ColourGradient grad (
            juce::Colour (0xFFF0E6CC), box.getCentreX(), box.getY(),
            juce::Colour (0xFFD9CFB2), box.getCentreX(), box.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (box, 2.0f);

        // Black-ink outline
        g.setColour (inkCol.withAlpha (0.85f));
        g.drawRoundedRectangle (box, 2.0f, 1.0f);

        // Down-arrow in dark ink
        const float ax = (float) (buttonX + buttonW * 0.5f);
        const float ay = (float) (buttonY + buttonH * 0.5f);
        juce::Path arrow;
        arrow.startNewSubPath (ax - 4.0f, ay - 2.0f);
        arrow.lineTo          (ax + 4.0f, ay - 2.0f);
        arrow.lineTo          (ax,        ay + 3.0f);
        arrow.closeSubPath();
        g.setColour (inkCol);
        g.fillPath (arrow);

        // Hairline separator before the arrow column
        g.setColour (inkCol.withAlpha (0.45f));
        g.drawLine ((float) buttonX, box.getY() + 3, (float) buttonX, box.getBottom() - 3, 0.7f);
    }

    //=========================================================================
    //  Panel painters
    //=========================================================================

    /** Anodised black-metal top deck — deep matte with subtle horizontal grain. */
    void InstructionCardLnF::drawPaperPanel (juce::Graphics& g, juce::Rectangle<int> r)
    {
        const auto bf = r.toFloat();

        // Base gradient: top slightly lifted, body deep matte black
        juce::ColourGradient grad (
            juce::Colour (0xFF2A2A2C), bf.getX(), bf.getY(),
            juce::Colour (0xFF0A0A0C), bf.getX(), bf.getBottom(), false);
        grad.addColour (0.10, juce::Colour (0xFF1E1E20));
        grad.addColour (0.65, juce::Colour (0xFF111114));
        g.setGradientFill (grad);
        g.fillRect (bf);

        // Horizontal brushed-metal grain (seeded random → stable repaint)
        juce::Random rng (987654321);
        for (int i = 0; i < 140; ++i)
        {
            const float fy    = bf.getY() + rng.nextFloat() * bf.getHeight();
            const float alpha = rng.nextFloat() * 0.05f + 0.005f;
            const bool  hi    = rng.nextBool();
            g.setColour ((hi ? juce::Colour (0xFF50505A) : juce::Colours::black).withAlpha (alpha));
            const float thickness = rng.nextFloat() * 0.6f + 0.2f;
            g.drawLine (bf.getX(), fy, bf.getRight(), fy, thickness);
        }

        // Top hairline highlight
        g.setColour (juce::Colour (0xFF6A6A72).withAlpha (0.55f));
        g.drawLine (bf.getX(), bf.getY() + 0.5f, bf.getRight(), bf.getY() + 0.5f, 0.8f);

        // Left vignette (deeper)
        {
            juce::ColourGradient vig (
                juce::Colours::black.withAlpha (0.45f), bf.getX(), bf.getCentreY(),
                juce::Colours::transparentBlack, bf.getX() + 80, bf.getCentreY(), false);
            g.setGradientFill (vig);
            g.fillRect (bf);
        }
        // Right vignette
        {
            juce::ColourGradient vig (
                juce::Colours::black.withAlpha (0.45f), bf.getRight(), bf.getCentreY(),
                juce::Colours::transparentBlack, bf.getRight() - 80, bf.getCentreY(), false);
            g.setGradientFill (vig);
            g.fillRect (bf);
        }
        // Bottom shadow (transition towards cream panel)
        {
            juce::ColourGradient vig (
                juce::Colours::transparentBlack, bf.getX(), bf.getBottom() - 30,
                juce::Colours::black.withAlpha (0.55f), bf.getX(), bf.getBottom(), false);
            g.setGradientFill (vig);
            g.fillRect (bf);
        }
    }

    /** Warm teak/walnut end-caps with procedural grain. */
    void InstructionCardLnF::drawWoodEndCap (juce::Graphics& g, juce::Rectangle<int> r, bool isLeft)
    {
        const auto bf = r.toFloat();

        // Base gradient — cross-grain (horizontal)
        juce::ColourGradient grad (
            isLeft ? teakLight() : teakDark(),
            bf.getX(), bf.getCentreY(),
            isLeft ? teakDark() : teakLight(),
            bf.getRight(), bf.getCentreY(), false);
        grad.addColour (0.4, teakMid());
        g.setGradientFill (grad);
        g.fillRect (bf);

        // Grain lines — thin dark verticals, seeded for stability
        juce::Random rng (isLeft ? 11111 : 22222);
        for (int i = 0; i < 14; ++i)
        {
            const float fx = bf.getX() + rng.nextFloat() * bf.getWidth();
            const float alpha = rng.nextFloat() * 0.35f + 0.05f;
            const float end   = fx + rng.nextFloat() * 1.5f - 0.75f;
            g.setColour (teakDark().withAlpha (alpha));
            g.drawLine (fx, bf.getY(), end, bf.getBottom(), 0.5f);
        }

        // Edge highlight (inner edge, lighter)
        const float edgeX = isLeft ? bf.getRight() - 1.5f : bf.getX() + 1.5f;
        juce::ColourGradient shine (
            teakLight().withAlpha (0.5f), edgeX, bf.getY(),
            juce::Colours::transparentBlack, edgeX, bf.getY() + 40, false);
        g.setGradientFill (shine);
        g.fillRect (isLeft ? bf.withTrimmedLeft (bf.getWidth() - 4) : bf.withWidth (4));
    }

    /** B&O instruction-card cream paper — the iconic look from under the lid. */
    void InstructionCardLnF::drawBlackPanel (juce::Graphics& g, juce::Rectangle<int> r)
    {
        const auto bf = r.toFloat();

        // Warm cream paper base
        juce::ColourGradient grad (
            juce::Colour (0xFFEFE6CE), bf.getCentreX(), bf.getY(),
            juce::Colour (0xFFD9CFB2), bf.getCentreX(), bf.getBottom(), false);
        grad.addColour (0.45, juce::Colour (0xFFE5DBC0));
        g.setGradientFill (grad);
        g.fillRect (bf);

        // Paper grain (faint stable noise)
        juce::Random rng (1968);
        for (int i = 0; i < 220; ++i)
        {
            const float fx = bf.getX() + rng.nextFloat() * bf.getWidth();
            const float fy = bf.getY() + rng.nextFloat() * bf.getHeight();
            const float a  = rng.nextFloat() * 0.06f + 0.01f;
            g.setColour ((rng.nextBool() ? juce::Colour (0xFF6A5A38) : juce::Colour (0xFFFFF6E0))
                            .withAlpha (a));
            g.fillEllipse (fx, fy, 0.7f, 0.7f);
        }

        // Vignette (paper edges slightly darker)
        juce::ColourGradient vig (
            juce::Colours::transparentBlack, bf.getCentreX(), bf.getCentreY(),
            juce::Colour (0xFF6A5A38).withAlpha (0.18f), bf.getX(), bf.getCentreY(), true);
        g.setGradientFill (vig);
        g.fillRect (bf);

        // Top hairline (separation from aluminium deck above)
        g.setColour (juce::Colour (0xFF3A2E18).withAlpha (0.55f));
        g.drawLine (bf.getX(), bf.getY() + 0.5f, bf.getRight(), bf.getY() + 0.5f, 0.8f);
    }

    /** Subtle section frame on dark panel. */
    void InstructionCardLnF::drawSectionBox (juce::Graphics& g, juce::Rectangle<int> r,
                                              const juce::String& title)
    {
        const auto bf = r.toFloat();
        g.setColour (panelEdge().withAlpha (0.5f));
        g.drawRect (bf, 0.7f);

        if (title.isNotEmpty())
        {
            const float tw = juce::jmax (36.0f, (float) title.length() * 6.0f);
            const auto tab = juce::Rectangle<float> (bf.getX() + 6, bf.getY() - 6, tw, 12);
            g.setColour (blackPanel());
            g.fillRect (tab);
            g.setColour (creamDim());
            g.setFont (sectionFont (8.0f));
            g.drawText (title, tab, juce::Justification::centred, false);
        }
    }

    //=========================================================================
    //  Beocord bullseye reel
    //    Rings (outside → in):  black rim | cream band | red ring | cream | hub | red dot
    //=========================================================================
    void InstructionCardLnF::drawReel (juce::Graphics& g, juce::Rectangle<int> r,
                                         float rotationRad, bool active)
    {
        // PHOTOREAL 3D REEL — built up like the real Beocord 2000 DL bullseye:
        // raised cream/red flange with depth shading, dark spoke openings (3),
        // chrome hub with screws, and tape wound on the centre.
        const auto bf = r.toFloat();
        const float cx = bf.getCentreX();
        const float cy = bf.getCentreY();
        const float radius = juce::jmin (bf.getWidth(), bf.getHeight()) * 0.5f - 3.0f;

        // ---- Drop shadow (sells separation from black metal) ----
        for (int i = 0; i < 3; ++i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.18f));
            const float pad = (float) (i + 1) * 1.5f;
            g.fillEllipse (cx - radius - pad, cy - radius - pad + 2,
                           (radius + pad) * 2, (radius + pad) * 2);
        }

        // ---- 1. Black outer rim (rubber tape-edge guard) ----
        juce::ColourGradient rimGrad (
            juce::Colour (0xFF1A1A1C), cx - radius * 0.4f, cy - radius * 0.4f,
            juce::Colour (0xFF050507), cx + radius * 0.4f, cy + radius * 0.4f, false);
        g.setGradientFill (rimGrad);
        g.fillEllipse (cx - radius, cy - radius, radius * 2, radius * 2);

        // Outer rim soft highlight (top-left light source)
        g.setColour (juce::Colour (0xFF505058).withAlpha (0.55f));
        g.drawEllipse (cx - radius, cy - radius, radius * 2, radius * 2, 1.2f);

        // ---- 2. Cream/silver outer flange band (raised) ----
        const float creamR = radius * 0.88f;
        juce::ColourGradient creamGrad (
            juce::Colour (0xFFF0E8C6), cx - creamR * 0.4f, cy - creamR * 0.4f,
            juce::Colour (0xFFB8B098), cx + creamR * 0.4f, cy + creamR * 0.4f, false);
        creamGrad.addColour (0.55, juce::Colour (0xFFD8D0B0));
        g.setGradientFill (creamGrad);
        g.fillEllipse (cx - creamR, cy - creamR, creamR * 2, creamR * 2);

        // ---- 3. RED ring (the iconic bullseye band) ----
        const float redR = radius * 0.72f;
        juce::ColourGradient redGrad (
            juce::Colour (0xFFE85040), cx - redR * 0.3f, cy - redR * 0.3f,
            juce::Colour (0xFF8C2018), cx + redR * 0.3f, cy + redR * 0.3f, false);
        redGrad.addColour (0.55, juce::Colour (0xFFC23A2A));
        g.setGradientFill (redGrad);
        g.fillEllipse (cx - redR, cy - redR, redR * 2, redR * 2);

        // ---- 4. Inner cream/silver flange (concentric, raised) ----
        const float innerR = radius * 0.54f;
        juce::ColourGradient innerGrad (
            juce::Colour (0xFFE8DEBE), cx - innerR * 0.3f, cy - innerR * 0.3f,
            juce::Colour (0xFFB0A888), cx + innerR * 0.3f, cy + innerR * 0.3f, false);
        innerGrad.addColour (0.5, juce::Colour (0xFFD0C8A8));
        g.setGradientFill (innerGrad);
        g.fillEllipse (cx - innerR, cy - innerR, innerR * 2, innerR * 2);

        // ---- 5. Three SPOKE WINDOWS (the dark openings showing tape behind) ----
        // These rotate with the reel — the iconic "3 holes" look.
        const float spokeOuterR = innerR * 0.92f;
        const float spokeInnerR = radius * 0.34f;
        for (int i = 0; i < 3; ++i)
        {
            const float a = rotationRad + (float) i * juce::MathConstants<float>::twoPi / 3.0f;
            const float halfW = juce::MathConstants<float>::pi * 0.10f;

            juce::Path spoke;
            spoke.addPieSegment (cx - spokeOuterR, cy - spokeOuterR,
                                  spokeOuterR * 2, spokeOuterR * 2,
                                  a - halfW, a + halfW, spokeInnerR / spokeOuterR);
            // Dark wound-tape colour visible through opening
            juce::ColourGradient tapeColour (
                juce::Colour (0xFF1A1812), cx, cy - spokeOuterR,
                juce::Colour (0xFF0A0808), cx, cy + spokeOuterR, false);
            g.setGradientFill (tapeColour);
            g.fillPath (spoke);
            // Subtle outline so the spoke reads as a window
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.strokePath (spoke, juce::PathStrokeType (0.7f));
        }

        // ---- 6. Wound-tape disc at centre (visible inside hub area) ----
        const float tapeR = radius * 0.34f;
        juce::ColourGradient tGrad (
            juce::Colour (0xFF2A241C), cx - tapeR * 0.4f, cy - tapeR * 0.4f,
            juce::Colour (0xFF080604), cx + tapeR * 0.4f, cy + tapeR * 0.4f, false);
        g.setGradientFill (tGrad);
        g.fillEllipse (cx - tapeR, cy - tapeR, tapeR * 2, tapeR * 2);

        // Subtle tape-wind concentric rings (visible "spool" look)
        g.setColour (juce::Colour (0xFF3A3024).withAlpha (0.45f));
        for (float rr = tapeR * 0.95f; rr > tapeR * 0.5f; rr -= tapeR * 0.07f)
            g.drawEllipse (cx - rr, cy - rr, rr * 2, rr * 2, 0.4f);

        // ---- 7. Chrome hub (the central drive cap) ----
        const float hubR = radius * 0.20f;
        juce::ColourGradient hubGrad (
            juce::Colour (0xFFE8E8EC), cx - hubR * 0.4f, cy - hubR * 0.5f,
            juce::Colour (0xFF40404A), cx + hubR * 0.4f, cy + hubR * 0.5f, false);
        hubGrad.addColour (0.5, juce::Colour (0xFF9A9AA0));
        g.setGradientFill (hubGrad);
        g.fillEllipse (cx - hubR, cy - hubR, hubR * 2, hubR * 2);

        g.setColour (juce::Colour (0xFF1A1A1C));
        g.drawEllipse (cx - hubR, cy - hubR, hubR * 2, hubR * 2, 1.0f);

        // ---- 8. Three small SCREWS on the hub (Phillips heads, rotate with reel) ----
        const float screwR = juce::jmax (1.2f, hubR * 0.18f);
        const float screwOrbit = hubR * 0.55f;
        for (int i = 0; i < 3; ++i)
        {
            const float a  = rotationRad + (float) i * juce::MathConstants<float>::twoPi / 3.0f
                              + juce::MathConstants<float>::pi / 6.0f;
            const float sx = cx + screwOrbit * std::sin (a);
            const float sy = cy - screwOrbit * std::cos (a);

            juce::ColourGradient sGrad (
                juce::Colour (0xFFE8E8EC), sx - screwR * 0.5f, sy - screwR * 0.5f,
                juce::Colour (0xFF505058), sx + screwR * 0.5f, sy + screwR * 0.5f, false);
            g.setGradientFill (sGrad);
            g.fillEllipse (sx - screwR, sy - screwR, screwR * 2, screwR * 2);
            g.setColour (juce::Colour (0xFF1A1A1C));
            g.drawEllipse (sx - screwR, sy - screwR, screwR * 2, screwR * 2, 0.5f);
            // Phillips slot (cross)
            g.drawLine (sx - screwR * 0.6f, sy, sx + screwR * 0.6f, sy, 0.8f);
            g.drawLine (sx, sy - screwR * 0.6f, sx, sy + screwR * 0.6f, 0.8f);
        }

        // ---- 9. Concentric separator outlines (subtle, sells depth) ----
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawEllipse (cx - creamR, cy - creamR, creamR * 2, creamR * 2, 0.6f);
        g.drawEllipse (cx - redR,   cy - redR,   redR * 2,   redR * 2,   0.5f);
        g.drawEllipse (cx - innerR, cy - innerR, innerR * 2, innerR * 2, 0.5f);

        // ---- 10. Specular highlight (top-left, sells curvature) ----
        juce::Path glint;
        glint.addCentredArc (cx, cy, radius * 0.95f, radius * 0.95f, 0,
                              -juce::MathConstants<float>::pi * 0.6f,
                              -juce::MathConstants<float>::pi * 0.15f, true);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.strokePath (glint, juce::PathStrokeType (3.0f));
    }

    //=========================================================================
    //  Head assembly (between reels)
    //=========================================================================
    void InstructionCardLnF::drawHeadAssembly (juce::Graphics& g, juce::Rectangle<int> r)
    {
        auto bf = r.toFloat().reduced (4.0f, 6.0f);

        const float w = bf.getWidth() / 3.0f;
        const juce::String labels[] = { "E", "R", "P" };
        for (int i = 0; i < 3; ++i)
        {
            auto headRect = juce::Rectangle<float> (
                bf.getX() + (float) i * w + 2.0f, bf.getY(), w - 4.0f, bf.getHeight());

            juce::ColourGradient grad (
                aluHi(), headRect.getX(), headRect.getY(),
                aluLo(), headRect.getX(), headRect.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (headRect, 1.0f);
            g.setColour (ink().withAlpha (0.6f));
            g.drawRoundedRectangle (headRect, 1.0f, 0.6f);

            g.setColour (ink());
            g.drawLine (headRect.getCentreX(), headRect.getY() + 2,
                        headRect.getCentreX(), headRect.getBottom() - 2, 1.0f);

            g.setColour (ink());
            g.setFont (sectionFont (7.5f));
            auto lblRect = headRect.removeFromBottom (10);
            g.drawText (labels[i], lblRect.toNearestInt(),
                        juce::Justification::centred, false);
        }

        // Capstan
        const float capR = bf.getHeight() * 0.16f;
        const float capX = bf.getRight() - capR * 1.5f;
        const float capY = bf.getCentreY();
        g.setColour (ink());
        g.fillEllipse (capX - capR, capY - capR, capR * 2, capR * 2);
        g.setColour (aluLo());
        g.fillEllipse (capX - capR * 0.5f, capY - capR * 0.5f, capR, capR);
    }

    //=========================================================================
    //  Transport key — dark hardware button
    //=========================================================================
    void InstructionCardLnF::drawTransportKey (juce::Graphics& g, juce::Rectangle<int> r,
                                                  bool down, bool active, juce::Colour accent)
    {
        // INSTRUCTION-CARD STYLE: pictogram-box, thin black ink outline on
        // cream paper. The button visibly depresses; an active key fills with
        // accent (red for REC, amber for SPK/MUTE).
        const auto bf = r.toFloat().reduced (1.0f);
        const auto inkCol = juce::Colour (0xFF181408);
        const float keyOffset = down ? 1.0f : 0.0f;
        const auto keyRect = bf.translated (0, keyOffset);

        // ---- Soft drop shadow when raised ----
        if (! down)
        {
            g.setColour (inkCol.withAlpha (0.20f));
            g.fillRoundedRectangle (bf.translated (0, 1.5f), 2.5f);
        }

        // ---- Body fill ----
        if (active)
        {
            juce::ColourGradient g1 (
                accent.brighter (0.20f), keyRect.getCentreX(), keyRect.getY(),
                accent.darker (0.20f),   keyRect.getCentreX(), keyRect.getBottom(), false);
            g.setGradientFill (g1);
            g.fillRoundedRectangle (keyRect, 2.5f);
        }
        else
        {
            // Cream paper key with subtle edge gradient (raised)
            juce::ColourGradient g1 (
                juce::Colour (0xFFF2E8CE), keyRect.getCentreX(), keyRect.getY(),
                juce::Colour (0xFFCFC4A6), keyRect.getCentreX(), keyRect.getBottom(), false);
            g.setGradientFill (g1);
            g.fillRoundedRectangle (keyRect, 2.5f);
        }

        // ---- Black-ink outline ----
        g.setColour (inkCol.withAlpha (0.85f));
        g.drawRoundedRectangle (keyRect, 2.5f, 1.0f);

        // ---- Top hairline highlight (sells "raised") ----
        if (! down && ! active)
        {
            g.setColour (juce::Colours::white.withAlpha (0.5f));
            g.drawLine (keyRect.getX() + 2, keyRect.getY() + 0.7f,
                        keyRect.getRight() - 2, keyRect.getY() + 0.7f, 0.6f);
        }
    }

    //=========================================================================
    //  Arrow label + title + counter
    //=========================================================================
    void InstructionCardLnF::drawArrowLabel (juce::Graphics& g, juce::Rectangle<int> r,
                                                const juce::String& text, bool arrowRight)
    {
        const auto bf = r.toFloat();
        g.setColour (creamLabel());
        g.setFont (labelFont (10.0f));

        if (arrowRight)
        {
            g.drawText (text, bf.withTrimmedRight (10).toNearestInt(),
                        juce::Justification::centredRight, false);
            juce::Path arrow;
            const float ax = bf.getRight() - 4;
            const float ay = bf.getCentreY();
            arrow.startNewSubPath (ax - 5, ay - 3);
            arrow.lineTo          (ax, ay);
            arrow.lineTo          (ax - 5, ay + 3);
            g.strokePath (arrow, juce::PathStrokeType (0.9f));
        }
        else
        {
            g.drawText (text, bf.withTrimmedLeft (10).toNearestInt(),
                        juce::Justification::centredLeft, false);
            juce::Path arrow;
            const float ax = bf.getX() + 4;
            const float ay = bf.getCentreY();
            arrow.startNewSubPath (ax + 5, ay - 3);
            arrow.lineTo          (ax, ay);
            arrow.lineTo          (ax + 5, ay + 3);
            g.strokePath (arrow, juce::PathStrokeType (0.9f));
        }
    }

    /** Title drawn on black-metal top deck — silver-engraved B&O style. */
    void InstructionCardLnF::drawTitle (juce::Graphics& g, juce::Rectangle<int> r,
                                          const juce::String& title,
                                          const juce::String& subtitle)
    {
        auto bf = r.toFloat();

        // Engraved-look main title — bright silver with deep shadow under
        auto tr = bf.removeFromTop (26).toNearestInt();
        g.setFont (logoFont (22.0f));
        // bottom shadow (etched effect into the metal)
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.drawText (title, tr.translated (0, 1), juce::Justification::left, false);
        // bright fill
        g.setColour (juce::Colour (0xFFE8E8EC));
        g.drawText (title, tr, juce::Justification::left, false);

        if (subtitle.isNotEmpty())
        {
            g.setColour (juce::Colour (0xFFB0B0B6).withAlpha (0.75f));
            g.setFont (sectionFont (9.0f));
            g.drawText (subtitle, bf.toNearestInt(),
                         juce::Justification::topLeft, false);
        }
    }

    //=========================================================================
    //  Analog VU meter — proper hardware look with curved scale and needle
    //=========================================================================
    void InstructionCardLnF::drawAnalogVU (juce::Graphics& g, juce::Rectangle<int> r,
                                             float dbValue, bool isPeaking,
                                             const juce::String& channel)
    {
        const auto bf = r.toFloat().reduced (1.0f);
        const auto inkCol = juce::Colour (0xFF181408);

        // ---- Outer black-metal bezel ----
        juce::ColourGradient bezelGrad (
            juce::Colour (0xFF2C2C30), bf.getCentreX(), bf.getY(),
            juce::Colour (0xFF050507), bf.getCentreX(), bf.getBottom(), false);
        g.setGradientFill (bezelGrad);
        g.fillRoundedRectangle (bf, 4.0f);

        // Bezel rim highlight
        g.setColour (juce::Colour (0xFF6A6A72).withAlpha (0.7f));
        g.drawRoundedRectangle (bf, 4.0f, 1.0f);

        // ---- Cream-coloured meter face (recessed) ----
        const auto face = bf.reduced (5.0f, 5.0f);
        juce::ColourGradient faceGrad (
            juce::Colour (0xFFF5EBD0), face.getCentreX(), face.getY(),
            juce::Colour (0xFFD8CDB0), face.getCentreX(), face.getBottom(), false);
        g.setGradientFill (faceGrad);
        g.fillRoundedRectangle (face, 2.5f);

        // Face inner shadow at top (recessed look)
        juce::ColourGradient shadow (
            juce::Colours::black.withAlpha (0.30f), face.getCentreX(), face.getY(),
            juce::Colours::transparentBlack,         face.getCentreX(), face.getY() + 6.0f, false);
        g.setGradientFill (shadow);
        g.fillRoundedRectangle (face, 2.5f);

        // ---- Needle pivot (bottom-centre, well below the visible face) ----
        const float pivotX = face.getCentreX();
        const float pivotY = face.getBottom() + face.getHeight() * 0.35f;
        const float arcR   = face.getHeight() * 1.05f;

        // ---- Scale arc (curved scale at top of face) ----
        const float angSpan = juce::MathConstants<float>::pi * 0.42f;  // ~75°
        const float ang0 = -angSpan * 0.5f;
        const float ang1 =  angSpan * 0.5f;

        // Black scale arc line
        juce::Path scaleArc;
        scaleArc.addCentredArc (pivotX, pivotY, arcR, arcR, 0, ang0, ang1, true);
        g.setColour (inkCol);
        g.strokePath (scaleArc, juce::PathStrokeType (1.0f));

        // Red zone (after 0 dB, ~75% to 100%)
        const float redStart = ang0 + (ang1 - ang0) * 0.75f;
        juce::Path redArc;
        redArc.addCentredArc (pivotX, pivotY, arcR, arcR, 0, redStart, ang1, true);
        g.setColour (juce::Colour (0xFFC23A2A));
        g.strokePath (redArc, juce::PathStrokeType (2.4f));

        // Major tick marks at -20, -10, -5, -3, 0, +1, +3
        const float ticks[] = { 0.0f, 0.20f, 0.45f, 0.60f, 0.75f, 0.85f, 1.0f };
        const char* lbls[]  = { "-20", "-10", "-5", "-3", "0", "+1", "+3" };
        g.setFont (sectionFont (7.5f));
        for (int i = 0; i < 7; ++i)
        {
            const float t = ticks[i];
            const float a = ang0 + t * (ang1 - ang0);
            const float r1 = arcR;
            const float r2 = arcR - 5.0f;
            const float x1 = pivotX + r1 * std::sin (a);
            const float y1 = pivotY - r1 * std::cos (a);
            const float x2 = pivotX + r2 * std::sin (a);
            const float y2 = pivotY - r2 * std::cos (a);
            g.setColour (t >= 0.75f ? juce::Colour (0xFFC23A2A) : inkCol);
            g.drawLine (x1, y1, x2, y2, 1.2f);

            // Label
            const float r3 = arcR - 11.0f;
            const float lx = pivotX + r3 * std::sin (a) - 9;
            const float ly = pivotY - r3 * std::cos (a) - 5;
            g.drawText (lbls[i], (int) lx, (int) ly, 18, 10,
                         juce::Justification::centred, false);
        }

        // Minor ticks (between majors)
        g.setColour (inkCol.withAlpha (0.6f));
        for (int i = 1; i < 20; ++i)
        {
            const float t = (float) i / 20.0f;
            // Skip near majors
            bool nearMajor = false;
            for (float mt : ticks)
                if (std::abs (mt - t) < 0.025f) { nearMajor = true; break; }
            if (nearMajor) continue;

            const float a = ang0 + t * (ang1 - ang0);
            const float r1 = arcR;
            const float r2 = arcR - 3.0f;
            g.drawLine (pivotX + r1 * std::sin (a), pivotY - r1 * std::cos (a),
                        pivotX + r2 * std::sin (a), pivotY - r2 * std::cos (a), 0.6f);
        }

        // VU + channel label
        g.setColour (inkCol);
        g.setFont (logoFont (10.0f));
        g.drawText ("VU", face.toNearestInt().withTrimmedTop ((int) (face.getHeight() * 0.55f)),
                     juce::Justification::centred, false);
        g.setFont (sectionFont (7.5f));
        g.setColour (inkCol.withAlpha (0.7f));
        g.drawText (channel, face.toNearestInt().withTrimmedTop ((int) (face.getHeight() * 0.72f)),
                     juce::Justification::centred, false);

        // ---- Needle ----
        const float vuNorm = juce::jlimit (0.0f, 1.0f, (dbValue + 20.0f) / 23.0f);
        const float needleAng = ang0 + vuNorm * (ang1 - ang0);
        const float nLen = arcR - 1.0f;
        const float nx = pivotX + nLen * std::sin (needleAng);
        const float ny = pivotY - nLen * std::cos (needleAng);

        // Needle shadow
        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.drawLine (pivotX + 1.0f, pivotY + 1.0f, nx + 1.0f, ny + 1.0f, 1.6f);

        // Needle body
        g.setColour (inkCol);
        g.drawLine (pivotX, pivotY, nx, ny, 1.4f);

        // Needle pivot cap (small black disc at base of visible area)
        g.fillEllipse (pivotX - 3.0f, face.getBottom() - 4.0f, 6.0f, 6.0f);
        g.setColour (juce::Colour (0xFF6A6A72));
        g.drawEllipse (pivotX - 3.0f, face.getBottom() - 4.0f, 6.0f, 6.0f, 0.6f);

        // ---- Glass sheen (subtle reflection) ----
        juce::ColourGradient glass (
            juce::Colours::white.withAlpha (0.18f), face.getX(), face.getY(),
            juce::Colours::transparentWhite,         face.getX(), face.getCentreY(), false);
        g.setGradientFill (glass);
        g.fillRoundedRectangle (face, 2.5f);

        // ---- PEAK dot (red LED, top-right of face, lights when isPeaking) ----
        if (isPeaking)
        {
            const float pR = 3.0f;
            const float px = face.getRight() - pR * 2 - 3;
            const float py = face.getY() + pR + 3;
            // glow
            g.setColour (juce::Colour (0xFFE85040).withAlpha (0.4f));
            g.fillEllipse (px - pR * 2.2f, py - pR * 2.2f, pR * 4.4f, pR * 4.4f);
            g.setColour (juce::Colour (0xFFFF6050));
            g.fillEllipse (px - pR, py - pR, pR * 2, pR * 2);
            g.setColour (juce::Colours::white.withAlpha (0.6f));
            g.fillEllipse (px - pR * 0.4f, py - pR * 0.6f, pR * 0.8f, pR * 0.6f);
        }
    }

    /** Counter: amber digits in a dark recessed window. */
    void InstructionCardLnF::drawCounter (juce::Graphics& g, juce::Rectangle<int> r,
                                            const juce::String& text)
    {
        const auto bf = r.toFloat();

        g.setColour (juce::Colour (0xFF0A0A0C));
        g.fillRoundedRectangle (bf, 3.0f);
        g.setColour (panelEdge().withAlpha (0.6f));
        g.drawRoundedRectangle (bf, 3.0f, 0.8f);

        // Inner highlight at top
        juce::ColourGradient shine (
            juce::Colour (0xFF303038).withAlpha (0.6f), bf.getX(), bf.getY(),
            juce::Colours::transparentBlack,             bf.getX(), bf.getY() + bf.getHeight() * 0.4f, false);
        g.setGradientFill (shine);
        g.fillRoundedRectangle (bf.reduced (1.0f), 2.0f);

        // Amber digits
        g.setColour (amberHot());
        g.setFont (monoFont (juce::jmax (10.0f, bf.getHeight() * 0.55f)));
        g.drawText (text, bf.toNearestInt(), juce::Justification::centred, false);
    }
}
