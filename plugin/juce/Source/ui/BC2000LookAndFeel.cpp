/*  BC2000LookAndFeel implementation — hardware-accurate Beocord 2000 DL aesthetic. */

#include "BC2000LookAndFeel.h"

namespace bc2000dl::ui
{
    //=========================================================================
    //  Construction
    //=========================================================================
    InstructionCardLnF::InstructionCardLnF()
    {
        // Dark panel defaults (controls live on the black panel)
        setColour (juce::ResizableWindow::backgroundColourId, blackPanel());
        setColour (juce::DocumentWindow::backgroundColourId, blackPanel());
        setColour (juce::Label::textColourId, creamLabel());

        setColour (juce::TextEditor::backgroundColourId, panelMid());
        setColour (juce::TextEditor::textColourId, creamLabel());
        setColour (juce::TextEditor::outlineColourId, panelEdge());

        setColour (juce::ComboBox::backgroundColourId, panelMid());
        setColour (juce::ComboBox::textColourId, creamLabel());
        setColour (juce::ComboBox::outlineColourId, panelEdge());
        setColour (juce::ComboBox::arrowColourId, amber());

        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xFF1E1E22));
        setColour (juce::PopupMenu::textColourId, creamLabel());
        setColour (juce::PopupMenu::highlightedBackgroundColourId, redAccent());
        setColour (juce::PopupMenu::highlightedTextColourId, paper());

        setColour (juce::TextButton::buttonColourId, panelMid());
        setColour (juce::TextButton::textColourOffId, creamLabel());
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);

        setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xFF1E1E22));
        setColour (juce::TooltipWindow::textColourId, creamLabel());
        setColour (juce::TooltipWindow::outlineColourId, panelEdge());
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
        // Deep recessed slot — black ink, with a 1-px highlight on the left wall
        // so it reads as a milled channel under raking light.
        g.setColour (juce::Colour (0xFF050507));
        g.fillRoundedRectangle (ch, 1.2f);

        // Inner shadow at top to imply depth
        juce::ColourGradient depthGrad (
            juce::Colours::black.withAlpha (0.85f), ch.getCentreX(), ch.getY(),
            juce::Colours::transparentBlack,         ch.getCentreX(), ch.getY() + 6.0f, false);
        g.setGradientFill (depthGrad);
        g.fillRoundedRectangle (ch, 1.2f);

        // Inner highlight (left edge) — light catches the milled wall
        g.setColour (juce::Colour (0xFF505058).withAlpha (0.55f));
        g.drawLine (ch.getX() + 0.6f, ch.getY() + 2, ch.getX() + 0.6f, ch.getBottom() - 2, 0.7f);

        // White-painted scale on both sides (every 10%, larger every 50%)
        // The real machine has these silk-screened on the panel.
        g.setColour (creamLabel().withAlpha (0.55f));
        for (int i = 0; i <= 10; ++i)
        {
            const float ty = ch.getY() + (ch.getHeight() * (float) i / 10.0f);
            const float tw = (i == 0 || i == 5 || i == 10) ? 7.0f : 4.0f;
            g.drawLine (ch.getX() - tw - 1.0f, ty, ch.getX() - 2.0f, ty, 0.7f);
            g.drawLine (ch.getRight() + 2.0f, ty, ch.getRight() + tw + 1.0f, ty, 0.7f);
        }
    }

    void InstructionCardLnF::drawSlideFaderCap (juce::Graphics& g, juce::Rectangle<float> r,
                                                  bool bright)
    {
        // ---- Drop shadow (sells depth, reads as a chunky thumb) ----
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (r.translated (0, 1.5f).expanded (0.5f, 0.5f), 3.0f);

        // ---- Body: brushed dark chrome with bright top/bottom edges ----
        juce::ColourGradient grad (
            juce::Colour (0xFF7A7A82),  r.getCentreX(), r.getY(),
            juce::Colour (0xFF26262A),  r.getCentreX(), r.getBottom(), false);
        grad.addColour (0.12, juce::Colour (0xFFA8A8B0));   // top highlight
        grad.addColour (0.50, juce::Colour (0xFF5A5A60));   // mid
        grad.addColour (0.88, juce::Colour (0xFF1A1A1E));   // bottom shadow
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 3.0f);

        // ---- Side knurling (vertical micro-grooves either side of the red bar) ----
        const float cy = r.getCentreY();
        g.setColour (juce::Colour (0xFF101012).withAlpha (0.55f));
        for (float dy : { -8.0f, -5.5f, -3.0f, 3.0f, 5.5f, 8.0f })
            g.drawLine (r.getX() + 2.5f, cy + dy, r.getRight() - 2.5f, cy + dy, 0.5f);
        g.setColour (juce::Colour (0xFFC8C8D0).withAlpha (0.20f));
        for (float dy : { -7.2f, -4.7f, -2.2f, 3.7f, 6.2f, 8.7f })
            g.drawLine (r.getX() + 2.5f, cy + dy, r.getRight() - 2.5f, cy + dy, 0.4f);

        // ---- Bright outline ----
        g.setColour (juce::Colour (0xFFB0B0B8).withAlpha (bright ? 0.85f : 0.55f));
        g.drawRoundedRectangle (r, 3.0f, 0.9f);

        // ---- THE iconic red position-marker bar across the centre ----
        const float barH = 2.5f;
        const auto bar = juce::Rectangle<float> (r.getX() + 1.5f, cy - barH * 0.5f,
                                                  r.getWidth() - 3.0f, barH);
        juce::ColourGradient barGrad (
            juce::Colour (0xFFE85040), bar.getCentreX(), bar.getY(),
            juce::Colour (0xFFA02818), bar.getCentreX(), bar.getBottom(), false);
        g.setGradientFill (barGrad);
        g.fillRoundedRectangle (bar, 0.8f);
        // Bar highlight
        g.setColour (juce::Colour (0xFFFF9080).withAlpha (0.6f));
        g.drawLine (bar.getX() + 1, bar.getY() + 0.5f, bar.getRight() - 1, bar.getY() + 0.5f, 0.6f);

        // ---- Hot ring when mouse over / dragging ----
        if (bright)
        {
            g.setColour (amber().withAlpha (0.30f));
            g.drawRoundedRectangle (r.expanded (0.8f), 3.5f, 1.2f);
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
        const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h)
                              .reduced (4.0f);
        const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
        const float cx = area.getCentreX();
        const float cy = area.getCentreY();
        const float angle = angleStart + sliderPos * (angleEnd - angleStart);
        const bool hot = s.isMouseOverOrDragging();

        // ---- Hover halo (amber, soft) ----
        if (hot)
        {
            g.setColour (amber().withAlpha (0.18f));
            g.fillEllipse (cx - radius - 2, cy - radius - 2,
                           (radius + 2) * 2, (radius + 2) * 2);
        }

        // ---- Arc track (recessed groove) ----
        const float trackR = radius - 1.5f;
        g.setColour (juce::Colour (0xFF080808));
        juce::Path arcPath;
        arcPath.addCentredArc (cx, cy, trackR, trackR, 0, angleStart, angleEnd, true);
        g.strokePath (arcPath, juce::PathStrokeType (3.0f));
        g.setColour (panelEdge().withAlpha (0.7f));
        g.strokePath (arcPath, juce::PathStrokeType (1.0f));

        // ---- Filled arc (amber value-arc) ----
        juce::Path arcFill;
        arcFill.addCentredArc (cx, cy, trackR, trackR, 0, angleStart, angle, true);
        g.setColour (amber().withAlpha (hot ? 0.95f : 0.75f));
        g.strokePath (arcFill, juce::PathStrokeType (2.4f));

        // Soft amber glow under hot
        if (hot)
        {
            g.setColour (amberHot().withAlpha (0.25f));
            g.strokePath (arcFill, juce::PathStrokeType (5.0f));
        }

        // ---- Outer chrome bezel ring (B&O brushed-aluminium) ----
        const float bezR = radius * 0.92f;
        juce::ColourGradient ringGrad (
            aluHi(), cx - bezR * 0.7f, cy - bezR * 0.7f,
            juce::Colour (0xFF6A6A6E), cx + bezR * 0.7f, cy + bezR * 0.7f, false);
        ringGrad.addColour (0.45, juce::Colour (0xFFC4C4C2));
        g.setGradientFill (ringGrad);
        g.fillEllipse (cx - bezR, cy - bezR, bezR * 2, bezR * 2);
        // Subtle bezel edge
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (cx - bezR, cy - bezR, bezR * 2, bezR * 2, 0.8f);

        // ---- Dark knob body (matte resin centre) ----
        const float bodyR = radius * 0.78f;
        juce::ColourGradient bodyGrad (
            juce::Colour (0xFF424246), cx, cy - bodyR,
            juce::Colour (0xFF161618), cx, cy + bodyR, false);
        bodyGrad.addColour (0.5, juce::Colour (0xFF2C2C30));
        g.setGradientFill (bodyGrad);
        g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2, bodyR * 2);

        // Knob top highlight (soft glint)
        juce::ColourGradient glint (
            juce::Colours::white.withAlpha (0.18f), cx, cy - bodyR + 1,
            juce::Colours::transparentWhite,         cx, cy, true);
        g.setGradientFill (glint);
        g.fillEllipse (cx - bodyR * 0.65f, cy - bodyR + 1.0f, bodyR * 1.3f, bodyR * 0.6f);

        // Body outline
        g.setColour (juce::Colour (0xFF000000).withAlpha (0.7f));
        g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2, bodyR * 2, 0.9f);

        // ---- Pointer line: red, prominent ----
        const float indR0 = bodyR * 0.18f;
        const float indR1 = bodyR * 0.92f;
        const auto pointerCol = s.isEnabled() ? redAccent() : creamDim();
        // Black underlay so the red reads against any bezel angle
        g.setColour (juce::Colours::black);
        g.drawLine (cx + indR0 * std::sin (angle), cy - indR0 * std::cos (angle),
                    cx + indR1 * std::sin (angle), cy - indR1 * std::cos (angle), 3.4f);
        g.setColour (pointerCol);
        g.drawLine (cx + indR0 * std::sin (angle), cy - indR0 * std::cos (angle),
                    cx + indR1 * std::sin (angle), cy - indR1 * std::cos (angle), 2.0f);

        // ---- Centre cap (chrome rivet) ----
        const float capR = bodyR * 0.18f;
        juce::ColourGradient capGrad (
            aluHi(), cx, cy - capR,
            juce::Colour (0xFF4A4A50), cx, cy + capR, false);
        g.setGradientFill (capGrad);
        g.fillEllipse (cx - capR, cy - capR, capR * 2, capR * 2);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (cx - capR, cy - capR, capR * 2, capR * 2, 0.6f);
    }

    //=========================================================================
    //  Toggle button — dark panel, red when ON
    //=========================================================================
    void InstructionCardLnF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                                 bool highlighted, bool down)
    {
        // Style: rectangular lozenge button (the real machine has them that way).
        // Whole bounds is clickable — the LED lights when ON.
        const auto bounds = b.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = b.getToggleState();

        // ---- Body ----
        juce::ColourGradient bodyGrad (
            juce::Colour (0xFF383840), bounds.getCentreX(), bounds.getY(),
            juce::Colour (0xFF161618), bounds.getCentreX(), bounds.getBottom(), false);
        if (down) bodyGrad.addColour (0.5, juce::Colour (0xFF202024));
        g.setGradientFill (bodyGrad);
        g.fillRoundedRectangle (bounds, 3.0f);

        // ---- LED dot (left side) ----
        const float ledR = juce::jmin (4.0f, bounds.getHeight() * 0.18f);
        const float ledX = bounds.getX() + ledR + 4.0f;
        const float ledY = bounds.getCentreY();
        if (on)
        {
            // glow
            g.setColour (redAccent().withAlpha (0.45f));
            g.fillEllipse (ledX - ledR * 2.2f, ledY - ledR * 2.2f, ledR * 4.4f, ledR * 4.4f);
            g.setColour (juce::Colour (0xFFFF6050));
            g.fillEllipse (ledX - ledR, ledY - ledR, ledR * 2, ledR * 2);
            g.setColour (juce::Colours::white.withAlpha (0.6f));
            g.fillEllipse (ledX - ledR * 0.4f, ledY - ledR * 0.6f, ledR * 0.8f, ledR * 0.6f);
        }
        else
        {
            g.setColour (juce::Colour (0xFF1A1A1C));
            g.fillEllipse (ledX - ledR, ledY - ledR, ledR * 2, ledR * 2);
            g.setColour (juce::Colour (0xFF383840));
            g.drawEllipse (ledX - ledR, ledY - ledR, ledR * 2, ledR * 2, 0.6f);
        }

        // ---- Outline ----
        g.setColour (highlighted ? amber().withAlpha (0.55f) : panelEdge().withAlpha (0.85f));
        g.drawRoundedRectangle (bounds, 3.0f, highlighted ? 1.0f : 0.7f);

        // ---- Top highlight ----
        g.setColour (juce::Colour (0xFF6A6A72).withAlpha (0.5f));
        g.drawLine (bounds.getX() + 2, bounds.getY() + 0.5f,
                    bounds.getRight() - 2, bounds.getY() + 0.5f, 0.6f);

        // ---- Label (right of LED) ----
        auto textArea = bounds;
        textArea.removeFromLeft (ledR * 2 + 8.0f);
        g.setColour (on ? juce::Colour (0xFFFFE4A0) : creamLabel());
        g.setFont (sectionFont (9.5f));
        g.drawFittedText (b.getButtonText(), textArea.toNearestInt(),
                           juce::Justification::centredLeft, 1);
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

        g.setColour (panelMid());
        g.fillRoundedRectangle (box, 2.0f);
        g.setColour (panelEdge());
        g.drawRoundedRectangle (box, 2.0f, 0.7f);

        // Amber down-arrow
        const float ax = (float) (buttonX + buttonW * 0.5f);
        const float ay = (float) (buttonY + buttonH * 0.5f);
        juce::Path arrow;
        arrow.startNewSubPath (ax - 4.0f, ay - 2.0f);
        arrow.lineTo          (ax + 4.0f, ay - 2.0f);
        arrow.lineTo          (ax,        ay + 3.0f);
        arrow.closeSubPath();
        g.setColour (cb.findColour (juce::ComboBox::arrowColourId));
        g.fillPath (arrow);
    }

    //=========================================================================
    //  Panel painters
    //=========================================================================

    /** Brushed aluminium — horizontal stripe noise over a silver gradient. */
    void InstructionCardLnF::drawPaperPanel (juce::Graphics& g, juce::Rectangle<int> r)
    {
        const auto bf = r.toFloat();

        // Base gradient: lighter at top, darker at bottom (overhead-light on deck)
        juce::ColourGradient grad (
            juce::Colour (0xFFD0D0CE), bf.getX(), bf.getY(),
            juce::Colour (0xFF9A9A98), bf.getX(), bf.getBottom(), false);
        grad.addColour (0.30, juce::Colour (0xFFC4C4C2));
        grad.addColour (0.65, juce::Colour (0xFFAEAEAC));
        g.setGradientFill (grad);
        g.fillRect (bf);

        // Horizontal brush-mark stripes (seeded random → stable repaint)
        juce::Random rng (987654321);
        for (int i = 0; i < 100; ++i)
        {
            const float fy    = bf.getY() + rng.nextFloat() * bf.getHeight();
            const float alpha = rng.nextFloat() * 0.045f;
            const bool  hi    = rng.nextBool();
            g.setColour ((hi ? juce::Colours::white : juce::Colours::black).withAlpha (alpha));
            const float thickness = rng.nextFloat() * 0.5f + 0.2f;
            g.drawLine (bf.getX(), fy, bf.getRight(), fy, thickness);
        }

        // Left vignette
        {
            juce::ColourGradient vig (
                juce::Colours::black.withAlpha (0.14f), bf.getX(), bf.getCentreY(),
                juce::Colours::transparentBlack, bf.getX() + 70, bf.getCentreY(), false);
            g.setGradientFill (vig);
            g.fillRect (bf);
        }
        // Right vignette
        {
            juce::ColourGradient vig (
                juce::Colours::black.withAlpha (0.14f), bf.getRight(), bf.getCentreY(),
                juce::Colours::transparentBlack, bf.getRight() - 70, bf.getCentreY(), false);
            g.setGradientFill (vig);
            g.fillRect (bf);
        }
        // Bottom shadow (transition towards black panel)
        {
            juce::ColourGradient vig (
                juce::Colours::transparentBlack, bf.getX(), bf.getBottom() - 30,
                juce::Colours::black.withAlpha (0.22f), bf.getX(), bf.getBottom(), false);
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

    /** Solid dark control-panel background with subtle top bevel. */
    void InstructionCardLnF::drawBlackPanel (juce::Graphics& g, juce::Rectangle<int> r)
    {
        const auto bf = r.toFloat();
        g.setColour (blackPanel());
        g.fillRect (bf);

        // Top highlight (1 px bevel at the aluminium/panel boundary)
        g.setColour (juce::Colour (0xFF484850).withAlpha (0.6f));
        g.drawLine (bf.getX(), bf.getY() + 0.5f, bf.getRight(), bf.getY() + 0.5f, 1.0f);

        // Very subtle radial vignette (centre brighter)
        juce::ColourGradient vig (
            juce::Colours::transparentBlack, bf.getCentreX(), bf.getCentreY(),
            juce::Colours::black.withAlpha (0.25f), bf.getX(), bf.getCentreY(), true);
        g.setGradientFill (vig);
        g.fillRect (bf);
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
        const auto bf = r.toFloat();
        const float cx = bf.getCentreX();
        const float cy = bf.getCentreY();
        const float radius = juce::jmin (bf.getWidth(), bf.getHeight()) * 0.5f - 2.0f;

        // ---- 1. Black outer rim (rubber tape contact) ----
        g.setColour (juce::Colour (0xFF0C0C0E));
        g.fillEllipse (cx - radius, cy - radius, radius * 2, radius * 2);

        // Outer rim highlight
        g.setColour (juce::Colour (0xFF484850).withAlpha (0.4f));
        g.drawEllipse (cx - radius, cy - radius, radius * 2, radius * 2, 1.0f);

        // ---- 2. Cream / silver band ----
        const float creamR = radius * 0.86f;
        g.setColour (juce::Colour (0xFFD8D0B0));
        g.fillEllipse (cx - creamR, cy - creamR, creamR * 2, creamR * 2);

        // ---- 3. Red ring ----
        const float redR = radius * 0.70f;
        g.setColour (redAccent());
        g.fillEllipse (cx - redR, cy - redR, redR * 2, redR * 2);

        // ---- 4. Inner cream circle ----
        const float innerR = radius * 0.54f;
        g.setColour (juce::Colour (0xFFDDD8C0));
        g.fillEllipse (cx - innerR, cy - innerR, innerR * 2, innerR * 2);

        // ---- 5. Hub (dark metallic, slight gradient) ----
        const float hubR = radius * 0.30f;
        juce::ColourGradient hubGrad (
            juce::Colour (0xFF8A8A88), cx, cy - hubR,
            juce::Colour (0xFF484846), cx, cy + hubR, false);
        g.setGradientFill (hubGrad);
        g.fillEllipse (cx - hubR, cy - hubR, hubR * 2, hubR * 2);

        g.setColour (juce::Colour (0xFF1A1A1C));
        g.drawEllipse (cx - hubR, cy - hubR, hubR * 2, hubR * 2, 1.0f);

        // ---- 6. Three pin holes (rotate when active) ----
        const float holeR = juce::jmax (1.5f, hubR * 0.22f);
        const float orbit = hubR * 0.60f;
        for (int i = 0; i < 3; ++i)
        {
            const float a  = rotationRad + (float) i * juce::MathConstants<float>::twoPi / 3.0f;
            const float hx = cx + orbit * std::sin (a);
            const float hy = cy - orbit * std::cos (a);
            g.setColour (juce::Colour (0xFF0A0A0C));
            g.fillEllipse (hx - holeR, hy - holeR, holeR * 2, holeR * 2);
        }

        // ---- 7. Red centre dot ----
        const float dotR = juce::jmax (2.0f, radius * 0.075f);
        g.setColour (redAccent());
        g.fillEllipse (cx - dotR, cy - dotR, dotR * 2, dotR * 2);

        // ---- 8. Thin separating outlines (cream band / red ring / inner cream) ----
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawEllipse (cx - creamR, cy - creamR, creamR * 2, creamR * 2, 0.7f);
        g.drawEllipse (cx - redR,   cy - redR,   redR * 2,   redR * 2,   0.5f);
        g.drawEllipse (cx - innerR, cy - innerR, innerR * 2, innerR * 2, 0.5f);

        // ---- 9. Tape wind animation (faint radial lines when active) ----
        if (active)
        {
            const float r1 = innerR * 0.96f;
            const float r0 = hubR * 1.10f;
            g.setColour (juce::Colours::white.withAlpha (0.07f));
            for (int i = 0; i < 8; ++i)
            {
                const float a = rotationRad + (float) i * juce::MathConstants<float>::pi / 4.0f;
                g.drawLine (cx + r0 * std::sin (a), cy - r0 * std::cos (a),
                            cx + r1 * std::sin (a), cy - r1 * std::cos (a), 0.7f);
            }
        }
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
        const auto bf = r.toFloat().reduced (1.0f);
        const float keyOffset = down ? 1.5f : 0.0f;
        const auto keyRect = bf.translated (0, keyOffset);

        // ---- Drop shadow under raised key ----
        if (! down)
        {
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillRoundedRectangle (bf.translated (0, 2.0f), 3.0f);
        }

        // ---- Body: piano-key style, dark satin metal ----
        juce::ColourGradient grad (
            juce::Colour (0xFF54545C), keyRect.getCentreX(), keyRect.getY(),
            juce::Colour (0xFF1C1C20), keyRect.getCentreX(), keyRect.getBottom(), false);
        grad.addColour (0.20, juce::Colour (0xFF42424A));
        grad.addColour (0.65, juce::Colour (0xFF2C2C32));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (keyRect, 3.0f);

        // ---- Top highlight (thin bright edge) ----
        g.setColour (juce::Colour (0xFF8A8A92).withAlpha (0.7f));
        g.drawLine (keyRect.getX() + 3, keyRect.getY() + 0.7f,
                    keyRect.getRight() - 3, keyRect.getY() + 0.7f, 0.8f);

        // ---- Active indicator strip (lit when ON) ----
        if (active)
        {
            const auto strip = juce::Rectangle<float> (
                keyRect.getX() + 5, keyRect.getY() + 3,
                keyRect.getWidth() - 10, 3.0f);
            // Glow halo
            g.setColour (accent.withAlpha (0.40f));
            g.fillRoundedRectangle (strip.expanded (1.5f, 1.0f), 2.5f);
            // Strip
            juce::ColourGradient sg (
                accent.brighter (0.4f), strip.getCentreX(), strip.getY(),
                accent.darker (0.2f),    strip.getCentreX(), strip.getBottom(), false);
            g.setGradientFill (sg);
            g.fillRoundedRectangle (strip, 1.5f);
        }

        // ---- Outline ----
        g.setColour (juce::Colour (0xFF000000).withAlpha (0.6f));
        g.drawRoundedRectangle (keyRect, 3.0f, 0.7f);
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

    /** Title drawn on brushed aluminium background — Bang & Olufsen style. */
    void InstructionCardLnF::drawTitle (juce::Graphics& g, juce::Rectangle<int> r,
                                          const juce::String& title,
                                          const juce::String& subtitle)
    {
        auto bf = r.toFloat();

        // Engraved-look main title (slight white halo above + dark fill below)
        auto tr = bf.removeFromTop (26).toNearestInt();
        g.setFont (logoFont (22.0f));
        // top highlight (etched effect)
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawText (title, tr.translated (0, -1), juce::Justification::left, false);
        // dark fill
        g.setColour (juce::Colour (0xFF181816));
        g.drawText (title, tr, juce::Justification::left, false);

        if (subtitle.isNotEmpty())
        {
            g.setColour (ink().withAlpha (0.65f));
            g.setFont (sectionFont (9.0f));
            g.drawText (subtitle, bf.toNearestInt(),
                         juce::Justification::topLeft, false);
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
