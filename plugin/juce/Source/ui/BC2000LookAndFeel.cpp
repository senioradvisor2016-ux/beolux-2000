/*  BC2000LookAndFeel implementation — hardware-accurate Beocord 2000 DL aesthetic. */

#include "BC2000LookAndFeel.h"
#include <melatonin_blur/melatonin_blur.h>

namespace bc2000dl::ui
{
    //=========================================================================
    //  Construction
    //=========================================================================
    InstructionCardLnF::InstructionCardLnF()
    {
        // BEOCORD 2400 BLACK PANEL defaults — black anodised aluminium body,
        // white silkscreen labels, white-faced buttons with dark text.
        const auto white    = juce::Colour (0xFFE8E8EA);
        const auto darkBg   = juce::Colour (0xFF101012);

        setColour (juce::ResizableWindow::backgroundColourId, darkBg);
        setColour (juce::DocumentWindow::backgroundColourId, darkBg);
        setColour (juce::Label::textColourId, white);

        setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xFF1A1A1E));
        setColour (juce::TextEditor::textColourId, white);
        setColour (juce::TextEditor::outlineColourId, juce::Colour (0xFF404048));

        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF181820));
        setColour (juce::ComboBox::textColourId, white);
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF404048));
        setColour (juce::ComboBox::arrowColourId, white);

        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xFF1A1A1E));
        setColour (juce::PopupMenu::textColourId, white);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xFF50505A));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        setColour (juce::TextButton::buttonColourId, juce::Colour (0xFFE0E0E0));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF101012));
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);

        setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xFF1A1A1E));
        setColour (juce::TooltipWindow::textColourId, white);
        setColour (juce::TooltipWindow::outlineColourId, juce::Colour (0xFF404048));
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
        // BEOCORD 2400 STYLE: deep recessed slot in black anodised panel,
        // with WHITE silkscreen tick marks AND numbers 0..10 alongside.
        g.setColour (juce::Colour (0xFF000000));
        g.fillRoundedRectangle (ch, 1.2f);

        // Real Gaussian inner shadow (sells the milled-channel depth)
        {
            juce::Path slot;
            slot.addRoundedRectangle (ch, 1.2f);
            static thread_local melatonin::InnerShadow channelInner {
                { juce::Colours::black.withAlpha (0.85f), 4, { 0, 1 }, 0 }
            };
            channelInner.render (g, slot);
        }

        // Inner-edge highlight
        g.setColour (juce::Colour (0xFF383840).withAlpha (0.7f));
        g.drawLine (ch.getX() + 0.5f, ch.getY() + 2, ch.getX() + 0.5f, ch.getBottom() - 2, 0.7f);

        // Channel outline
        g.setColour (juce::Colour (0xFF1E1E20));
        g.drawRoundedRectangle (ch, 1.2f, 0.6f);

        // Tick marks + 0..10 number labels (vertically: 10 at TOP, 0 at BOTTOM
        // matching the original Beocord 2400 — slider value increases upwards).
        const auto silkCol = juce::Colour (0xFFE8E8EA).withAlpha (0.85f);
        g.setFont (sectionFont (7.5f));
        for (int i = 0; i <= 10; ++i)
        {
            // i==0 shown as "10" at top of slider, i==10 as "0" at bottom
            const int label = 10 - i;
            const float ty = ch.getY() + (ch.getHeight() * (float) i / 10.0f);
            const float tw = (i == 0 || i == 5 || i == 10) ? 7.0f : 4.5f;
            g.setColour (silkCol);
            g.drawLine (ch.getX() - tw - 1.0f, ty, ch.getX() - 2.0f, ty, 0.85f);
            g.drawLine (ch.getRight() + 2.0f, ty, ch.getRight() + tw + 1.0f, ty, 0.85f);

            // Numbers next to long ticks (0, 5, 10) — left side only to avoid clutter
            if (i == 0 || i == 5 || i == 10)
            {
                g.drawText (juce::String (label),
                    juce::Rectangle<float> (ch.getX() - 22.0f, ty - 5.0f, 12.0f, 10.0f).toNearestInt(),
                    juce::Justification::centredRight, false);
            }
        }
    }

    void InstructionCardLnF::drawSlideFaderCap (juce::Graphics& g, juce::Rectangle<float> r,
                                                  bool bright)
    {
        // PREMIUM MATTE-ALU FADER CAP — anodised silver-grey body with subtle
        // top sheen, brushed grain, recessed red marker, deep drop shadow.

        // ---- Real Gaussian drop-shadow (melatonin) ----
        {
            juce::Path capPath;
            capPath.addRoundedRectangle (r, 2.5f);
            static thread_local melatonin::DropShadow capShadow {
                { juce::Colours::black.withAlpha (0.55f), 6, { 0, 3 }, 0 }
            };
            capShadow.render (g, capPath);
        }

        // ---- Anodised aluminium body (multi-stop satin gradient) ----
        juce::ColourGradient grad (
            juce::Colour (0xFFC8C8C2), r.getCentreX(), r.getY(),
            juce::Colour (0xFF7A7A74), r.getCentreX(), r.getBottom(), false);
        grad.addColour (0.18, juce::Colour (0xFFB8B8B2));
        grad.addColour (0.50, juce::Colour (0xFFA2A29C));
        grad.addColour (0.85, juce::Colour (0xFF888882));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 2.5f);

        // ---- Brushed-anodise grain (stable seeded noise) ----
        juce::Random rng (int (r.getY() * 17.0f) ^ int (r.getX() * 31.0f));
        for (int i = 0; i < 22; ++i)
        {
            const float fy = r.getY() + 1.5f + rng.nextFloat() * (r.getHeight() - 3.0f);
            const bool dark = rng.nextBool();
            g.setColour ((dark ? juce::Colour (0xFF6A6A64) : juce::Colour (0xFFE0E0DA))
                            .withAlpha (rng.nextFloat() * 0.10f + 0.02f));
            g.drawLine (r.getX() + 2.0f, fy, r.getRight() - 2.0f, fy, 0.4f);
        }

        // ---- Subtle glass-like top sheen (point-light reflection upper-left) ----
        {
            const float sheenH = r.getHeight() * 0.35f;
            const auto sheen = juce::Rectangle<float> (r.getX() + 2, r.getY() + 1,
                                                        r.getWidth() - 4, sheenH);
            juce::ColourGradient sg (
                juce::Colours::white.withAlpha (0.30f), sheen.getX(), sheen.getY(),
                juce::Colours::transparentWhite,         sheen.getX(), sheen.getBottom(), false);
            g.setGradientFill (sg);
            g.fillRoundedRectangle (sheen, 1.8f);
        }

        // ---- Bright top hairline highlight ----
        g.setColour (juce::Colour (0xFFF0F0EA).withAlpha (0.75f));
        g.drawLine (r.getX() + 2.5f, r.getY() + 0.7f,
                    r.getRight() - 2.5f, r.getY() + 0.7f, 0.7f);

        // ---- Bottom edge shadow ----
        g.setColour (juce::Colour (0xFF202020).withAlpha (0.45f));
        g.drawLine (r.getX() + 2.5f, r.getBottom() - 0.7f,
                    r.getRight() - 2.5f, r.getBottom() - 0.7f, 0.7f);

        // ---- Crisp dark outline ----
        g.setColour (juce::Colour (0xFF1C1C1A).withAlpha (0.90f));
        g.drawRoundedRectangle (r, 2.5f, 0.9f);

        // ---- Recessed red position-marker bar ----
        const float cy = r.getCentreY();
        const float barH = 2.4f;
        const auto bar = juce::Rectangle<float> (r.getX() + 3.0f, cy - barH * 0.5f,
                                                  r.getWidth() - 6.0f, barH);
        // Recess shadow (above the bar)
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawLine (bar.getX(), bar.getY() - 0.7f, bar.getRight(), bar.getY() - 0.7f, 0.7f);
        // Red paint inside recess
        juce::ColourGradient barGrad (
            juce::Colour (0xFFE03828), bar.getCentreX(), bar.getY(),
            juce::Colour (0xFF7C140C), bar.getCentreX(), bar.getBottom(), false);
        g.setGradientFill (barGrad);
        g.fillRect (bar);
        // Bar top highlight (sells "wet paint" look)
        g.setColour (juce::Colour (0xFFFF8070).withAlpha (0.55f));
        g.drawLine (bar.getX() + 0.5f, bar.getY() + 0.4f,
                    bar.getRight() - 0.5f, bar.getY() + 0.4f, 0.5f);

        // ---- Hot amber ring on hover/drag ----
        if (bright)
        {
            g.setColour (juce::Colour (0xFFC2A050).withAlpha (0.35f));
            g.drawRoundedRectangle (r.expanded (1.0f), 3.2f, 1.0f);
            g.setColour (juce::Colour (0xFFE0C070).withAlpha (0.20f));
            g.drawRoundedRectangle (r.expanded (2.5f), 3.8f, 1.5f);
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
        // ===================================================================
        //  UAD-GRADE KNOB
        //   1. Recessed skirt (knob sits IN the panel)
        //   2. Always-visible wide amber value-arc with track
        //   3. 12 perimeter tick-marks
        //   4. Multi-light PBR-style body shading
        //   5. Recessed triangular indicator nub at outer rim
        //   6. Centre chrome rivet
        // ===================================================================
        const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h)
                              .reduced (5.0f);
        const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
        const float cx = area.getCentreX();
        const float cy = area.getCentreY();
        const float angle = angleStart + sliderPos * (angleEnd - angleStart);
        const bool hot = s.isMouseOverOrDragging();

        const float bezR    = radius * 0.78f;             // bezel outer
        const float trackR  = radius * 0.94f;             // value-arc radius
        const float bodyR   = radius * 0.68f;             // body outer
        const auto  amberC  = juce::Colour (0xFFE8B040);
        const auto  amberHi = juce::Colour (0xFFFFD080);

        // -------------------------------------------------------------------
        // 1. RECESSED SKIRT (knob sits in a milled depression in the panel)
        // -------------------------------------------------------------------
        {
            juce::Path skirt;
            skirt.addEllipse (cx - radius, cy - radius, radius * 2, radius * 2);
            // outer drop shadow on the skirt rim
            static thread_local melatonin::DropShadow skirtShadow {
                { juce::Colours::black.withAlpha (0.55f), 6, { 0, 1 }, 0 }
            };
            skirtShadow.render (g, skirt);

            juce::ColourGradient skirtGrad (
                juce::Colour (0xFF050507), cx, cy - radius,
                juce::Colour (0xFF1A1A1E), cx, cy + radius, false);
            g.setGradientFill (skirtGrad);
            g.fillEllipse (cx - radius, cy - radius, radius * 2, radius * 2);

            // Inner shadow on the skirt — sells the recess
            static thread_local melatonin::InnerShadow skirtInner {
                { juce::Colours::black.withAlpha (0.85f), 4, { 0, 2 }, 0 }
            };
            skirtInner.render (g, skirt);
        }

        // -------------------------------------------------------------------
        // 2. VALUE-ARC TRACK (the always-visible UAD signature)
        // -------------------------------------------------------------------
        {
            // Background track (dark, slightly recessed)
            juce::Path bgArc;
            bgArc.addCentredArc (cx, cy, trackR, trackR, 0, angleStart, angleEnd, true);
            g.setColour (juce::Colour (0xFF202024));
            g.strokePath (bgArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));

            // Filled value-arc — amber, with melatonin glow when hot
            juce::Path valArc;
            valArc.addCentredArc (cx, cy, trackR, trackR, 0, angleStart, angle, true);

            if (hot)
            {
                static thread_local melatonin::DropShadow arcGlow {
                    { amberHi.withAlpha (0.85f), 8, { 0, 0 }, 1 }
                };
                arcGlow.render (g, valArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                                 juce::PathStrokeType::rounded));
            }

            g.setColour (hot ? amberHi : amberC);
            g.strokePath (valArc, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        }

        // -------------------------------------------------------------------
        // 3. 11 PERIMETER TICKS + scale numbers (Fairchild silkscreen scale)
        // -------------------------------------------------------------------
        {
            const float tickR1 = radius + 1.5f;
            const float tickR2 = radius + 5.0f;
            const float majorR2 = radius + 8.0f;
            const float numR = radius + 14.0f;     // distance for scale numbers
            g.setFont (sectionFont (8.5f));
            for (int i = 0; i <= 10; ++i)
            {
                const float t = (float) i / 10.0f;
                const float a = angleStart + t * (angleEnd - angleStart);
                const bool major = (i == 0 || i == 5 || i == 10);
                const float r2 = major ? majorR2 : tickR2;
                g.setColour (juce::Colour (0xFFE8E8EA).withAlpha (major ? 0.90f : 0.55f));
                g.drawLine (cx + tickR1 * std::sin (a), cy - tickR1 * std::cos (a),
                            cx + r2 * std::sin (a),     cy - r2 * std::cos (a),
                            major ? 1.3f : 0.7f);

                // Scale numbers at major positions
                if (major)
                {
                    const float nx = cx + numR * std::sin (a);
                    const float ny = cy - numR * std::cos (a);
                    juce::Rectangle<int> numBox ((int) nx - 10, (int) ny - 6, 20, 12);
                    // dark shadow under (silkscreen-into-panel emboss)
                    g.setColour (juce::Colours::black.withAlpha (0.80f));
                    g.drawText (juce::String (i), numBox.translated (0, 1),
                                juce::Justification::centred, false);
                    g.setColour (juce::Colour (0xFFE8E8EA));
                    g.drawText (juce::String (i), numBox,
                                juce::Justification::centred, false);
                }
            }
        }

        // -------------------------------------------------------------------
        // 4. BAKELITE BODY — Fairchild 670 / Pultec phenolic-resin knob
        //    Warm mahogany tones, glossy lacquer surface, subtle marbling.
        // -------------------------------------------------------------------
        {
            // Outer chrome bezel ring — Fairchild signature polished steel
            juce::ColourGradient bezelGrad (
                juce::Colour (0xFFE8E8EA), cx, cy - bezR,
                juce::Colour (0xFF40404A), cx, cy + bezR, false);
            bezelGrad.addColour (0.40, juce::Colour (0xFFC0C0C8));
            bezelGrad.addColour (0.65, juce::Colour (0xFF707078));
            g.setGradientFill (bezelGrad);
            g.fillEllipse (cx - bezR, cy - bezR, bezR * 2, bezR * 2);
            g.setColour (juce::Colours::black.withAlpha (0.85f));
            g.drawEllipse (cx - bezR, cy - bezR, bezR * 2, bezR * 2, 1.0f);

            // ---- ROUND GLOSSY PHENOLIC BODY (Fairchild 670 reference) ----
            // Pure circle — no flutes. The Fairchild aesthetic is in the
            // glossy black finish, sharp specular, and T-shaped grip pointer.
            juce::Path bodyShape;
            bodyShape.addEllipse (cx - bodyR, cy - bodyR, bodyR * 2, bodyR * 2);

            juce::ColourGradient bodyGrad (
                juce::Colour (0xFF3A3A3E), cx, cy - bodyR,
                juce::Colour (0xFF030305), cx, cy + bodyR, false);
            bodyGrad.addColour (0.18, juce::Colour (0xFF222226));
            bodyGrad.addColour (0.50, juce::Colour (0xFF101014));
            bodyGrad.addColour (0.85, juce::Colour (0xFF050508));
            g.setGradientFill (bodyGrad);
            g.fillPath (bodyShape);

            // ---- BEVEL HIGHLIGHT just inside chrome rim ----
            g.setColour (juce::Colour (0xFF8A8A92).withAlpha (0.85f));
            g.drawEllipse (cx - bodyR + 0.5f, cy - bodyR + 0.5f,
                           (bodyR - 0.5f) * 2, (bodyR - 0.5f) * 2, 0.8f);
            g.setColour (juce::Colours::black.withAlpha (0.85f));
            g.drawEllipse (cx - bodyR + 1.5f, cy - bodyR + 1.5f,
                           (bodyR - 1.5f) * 2, (bodyR - 1.5f) * 2, 0.6f);

            // ---- LIGHT 1: Sharp bright crescent (high-gloss lacquer) ----
            // Fairchild lacquer is intensely reflective — sharper highlight.
            {
                juce::Path glint;
                const float gR = bodyR * 0.97f;
                glint.addPieSegment (cx - gR, cy - gR, gR * 2, gR * 2,
                                      -juce::MathConstants<float>::pi * 0.62f,
                                      -juce::MathConstants<float>::pi * 0.14f,
                                      0.50f);
                juce::ColourGradient gg (
                    juce::Colours::white.withAlpha (0.65f),
                    cx - bodyR * 0.4f, cy - bodyR,
                    juce::Colours::transparentWhite,
                    cx, cy + bodyR * 0.05f, false);
                g.setGradientFill (gg);
                g.fillPath (glint);
            }

            // ---- LIGHT 2: Cool rim-light (top-right) ----
            {
                const float pR = bodyR * 0.32f;
                const float px = cx + bodyR * 0.50f;
                const float py = cy - bodyR * 0.30f;
                juce::ColourGradient bg (
                    juce::Colour (0xFFC0D0E0).withAlpha (0.22f), px, py,
                    juce::Colours::transparentWhite,             px + pR, py + pR, true);
                g.setGradientFill (bg);
                g.fillEllipse (px - pR, py - pR, pR * 2, pR * 2);
            }

            // ---- LIGHT 3: Subtle warm rim-light underside ----
            {
                juce::Path rimLight;
                const float gR = bodyR * 0.94f;
                rimLight.addPieSegment (cx - gR, cy - gR, gR * 2, gR * 2,
                                         juce::MathConstants<float>::pi * 0.25f,
                                         juce::MathConstants<float>::pi * 0.75f,
                                         0.90f);
                g.setColour (juce::Colour (0xFFC0A080).withAlpha (0.16f));
                g.fillPath (rimLight);
            }

            // ---- POINT-LIGHT WET-GLINT — sharper, brighter Fairchild reflection ----
            {
                const float pgR = bodyR * 0.12f;
                const float pgX = cx - bodyR * 0.35f;
                const float pgY = cy - bodyR * 0.55f;
                juce::ColourGradient pg (
                    juce::Colours::white,                   pgX, pgY,
                    juce::Colours::transparentWhite,         pgX + pgR, pgY + pgR, true);
                g.setGradientFill (pg);
                g.fillEllipse (pgX - pgR, pgY - pgR, pgR * 2, pgR * 2);
            }
            // Tighter inner core (super-sharp catchlight)
            {
                const float pgR = bodyR * 0.045f;
                g.setColour (juce::Colours::white);
                g.fillEllipse (cx - bodyR * 0.36f - pgR, cy - bodyR * 0.56f - pgR,
                               pgR * 2, pgR * 2);
            }

            // (No additional ridges — the fluted polygon body provides
            // all the perimeter texture. Adding more would clutter.)

            // ---- Body inner-edge shadow (deeper now — sells faceted edge) ----
            static thread_local melatonin::InnerShadow bodyInner {
                { juce::Colours::black.withAlpha (0.65f), 5, { 0, 1 }, 0 }
            };
            bodyInner.render (g, bodyShape);
        }

        // -------------------------------------------------------------------
        // 5. T-SHAPED GRIP-POINTER — Fairchild 670 signature handle
        //    A long white stem extending past the rim, with a horizontal
        //    cross-bar at the tip forming a T. This is the iconic grip you
        //    pinch when turning the knob.
        // -------------------------------------------------------------------
        {
            const float stemBaseR = bodyR * 0.18f;
            const float stemTipR  = bodyR * 1.02f;
            const float crossLen  = bodyR * 0.42f;        // horizontal cross length
            const float stemThick = juce::jmax (2.6f, bodyR * 0.075f);
            const float crossThick= juce::jmax (2.2f, bodyR * 0.060f);

            // Stem direction (radial)
            const float sn = std::sin (angle);
            const float cs = std::cos (angle);
            const float bx = cx + stemBaseR * sn;
            const float by = cy - stemBaseR * cs;
            const float tx = cx + stemTipR * sn;
            const float ty = cy - stemTipR * cs;

            // Cross direction (perpendicular to stem)
            const float crossDx =  cs * crossLen * 0.5f;
            const float crossDy =  sn * crossLen * 0.5f;

            // ---- Drop shadow under the whole T (melatonin) ----
            juce::Path tShape;
            tShape.addLineSegment (juce::Line<float> (bx, by, tx, ty), stemThick);
            tShape.addLineSegment (juce::Line<float> (tx - crossDx, ty - crossDy,
                                                      tx + crossDx, ty + crossDy), crossThick);
            static thread_local melatonin::DropShadow tShadow {
                { juce::Colours::black.withAlpha (0.85f), 4, { 0, 1 }, 0 }
            };
            tShadow.render (g, tShape);

            // ---- Stem (white painted) ----
            g.setColour (juce::Colour (0xFFFAFAF4));
            g.drawLine (bx, by, tx, ty, stemThick);

            // ---- Cross-bar at tip ----
            g.drawLine (tx - crossDx, ty - crossDy,
                        tx + crossDx, ty + crossDy, crossThick);

            // Highlight on top edge of stem (sells curved 3D)
            g.setColour (juce::Colour (0xFFFFFFFF).withAlpha (0.55f));
            g.drawLine (bx, by, tx, ty, stemThick * 0.45f);

            // Highlight on top edge of cross-bar
            g.drawLine (tx - crossDx, ty - crossDy,
                        tx + crossDx, ty + crossDy, crossThick * 0.45f);

            // Red dot at the very centre of the cross-bar (the visual lock-on)
            const auto tipCol = s.isEnabled() ? redAccent() : creamDim();
            const float dotR = juce::jmax (1.6f, bodyR * 0.045f);
            juce::Path dotPath;
            dotPath.addEllipse (tx - dotR, ty - dotR, dotR * 2, dotR * 2);
            static thread_local melatonin::DropShadow tipGlow {
                { juce::Colour (0xFFFF5040).withAlpha (0.85f), 4, { 0, 0 }, 0 }
            };
            tipGlow.render (g, dotPath);
            g.setColour (tipCol);
            g.fillEllipse (tx - dotR, ty - dotR, dotR * 2, dotR * 2);
        }

        // -------------------------------------------------------------------
        // 6. CENTRE BRASS CAP — Fairchild signature aged-brass top button
        // -------------------------------------------------------------------
        {
            const float rivR = bodyR * 0.16f;
            // dark recess
            g.setColour (juce::Colours::black);
            g.fillEllipse (cx - rivR - 0.7f, cy - rivR - 0.7f,
                           (rivR + 0.7f) * 2, (rivR + 0.7f) * 2);
            // aged brass dome (warm yellow → deep amber)
            juce::ColourGradient rivGrad (
                juce::Colour (0xFFFFE090), cx - rivR * 0.4f, cy - rivR * 0.5f,
                juce::Colour (0xFF3A2008), cx + rivR * 0.4f, cy + rivR * 0.5f, false);
            rivGrad.addColour (0.45, juce::Colour (0xFFC88830));
            rivGrad.addColour (0.80, juce::Colour (0xFF6A4818));
            g.setGradientFill (rivGrad);
            g.fillEllipse (cx - rivR, cy - rivR, rivR * 2, rivR * 2);
            // outline
            g.setColour (juce::Colour (0xFF2A1808).withAlpha (0.7f));
            g.drawEllipse (cx - rivR, cy - rivR, rivR * 2, rivR * 2, 0.6f);
            // bright warm specular
            g.setColour (juce::Colour (0xFFFFFFD0).withAlpha (0.85f));
            g.fillEllipse (cx - rivR * 0.45f, cy - rivR * 0.6f, rivR * 0.55f, rivR * 0.40f);
        }
    }

    //=========================================================================
    //  Toggle button — dark panel, red when ON
    //=========================================================================
    void InstructionCardLnF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                                 bool highlighted, bool down)
    {
        // BEOCORD 2400 STYLE: white-faced rectangular keys with embossed black
        // labels. ON state shows a small red LED dot on the left.
        const auto bounds = b.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = b.getToggleState();

        // ---- Real Gaussian drop-shadow (melatonin) ----
        if (! down)
        {
            juce::Path keyPath;
            keyPath.addRoundedRectangle (bounds, 2.5f);
            static thread_local melatonin::DropShadow toggleShadow {
                { juce::Colours::black.withAlpha (0.65f), 5, { 0, 2 }, 0 }
            };
            toggleShadow.render (g, keyPath);
        }

        // ---- WHITE PLASTIC KEY FACE ----
        juce::ColourGradient body (
            juce::Colour (0xFFEEEEEE), bounds.getCentreX(), bounds.getY(),
            juce::Colour (0xFFB6B6B8), bounds.getCentreX(), bounds.getBottom(), false);
        body.addColour (0.20, juce::Colour (0xFFE0E0E0));
        body.addColour (0.55, juce::Colour (0xFFCFCFD0));
        if (down || highlighted)
            body = juce::ColourGradient (
                juce::Colour (0xFFCFCFD0), bounds.getCentreX(), bounds.getY(),
                juce::Colour (0xFF989898), bounds.getCentreX(), bounds.getBottom(), false);
        g.setGradientFill (body);
        g.fillRoundedRectangle (bounds, 2.5f);

        // ---- Inner shadow on top edge (sells moulded plastic curvature) ----
        {
            juce::Path keyPath;
            keyPath.addRoundedRectangle (bounds, 2.5f);
            static thread_local melatonin::InnerShadow toggleInner {
                { juce::Colours::black.withAlpha (0.18f), 3, { 0, -1 }, 0 }
            };
            toggleInner.render (g, keyPath);
        }

        // Top hairline highlight (sells "moulded plastic" look)
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.drawLine (bounds.getX() + 2, bounds.getY() + 0.7f,
                    bounds.getRight() - 2, bounds.getY() + 0.7f, 0.7f);
        // Bottom edge shadow
        g.setColour (juce::Colours::black.withAlpha (0.40f));
        g.drawLine (bounds.getX() + 2, bounds.getBottom() - 0.7f,
                    bounds.getRight() - 2, bounds.getBottom() - 0.7f, 0.7f);
        // Crisp dark outline
        g.setColour (juce::Colour (0xFF101012).withAlpha (0.85f));
        g.drawRoundedRectangle (bounds, 2.5f, 0.8f);

        // ---- Red LED dot when ON (left side) ----
        const float ledR = juce::jmin (3.0f, bounds.getHeight() * 0.20f);
        const float ledX = bounds.getX() + ledR + 4.5f;
        const float ledY = bounds.getCentreY();
        if (on)
        {
            // Real Gaussian glow halo
            juce::Path ledPath;
            ledPath.addEllipse (ledX - ledR, ledY - ledR, ledR * 2, ledR * 2);
            static thread_local melatonin::DropShadow ledGlow {
                { juce::Colour (0xFFFF6050).withAlpha (0.85f), 10, { 0, 0 }, 1 }
            };
            ledGlow.render (g, ledPath);
            // LED body
            juce::ColourGradient lg (
                juce::Colour (0xFFFFE070), ledX - ledR * 0.4f, ledY - ledR * 0.5f,
                juce::Colour (0xFFA01810), ledX + ledR * 0.4f, ledY + ledR * 0.5f, false);
            g.setGradientFill (lg);
            g.fillEllipse (ledX - ledR, ledY - ledR, ledR * 2, ledR * 2);
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.fillEllipse (ledX - ledR * 0.4f, ledY - ledR * 0.7f, ledR * 0.7f, ledR * 0.5f);
        }
        else
        {
            // Tiny recessed dot (LED off)
            g.setColour (juce::Colour (0xFF8A8A8A).withAlpha (0.6f));
            g.fillEllipse (ledX - ledR * 0.7f, ledY - ledR * 0.7f, ledR * 1.4f, ledR * 1.4f);
        }

        // ---- Embossed black label (right of LED) ----
        auto textArea = bounds.toNearestInt();
        textArea.removeFromLeft ((int) (ledR * 2 + 9.0f));
        g.setFont (sectionFont (9.5f));
        // Subtle white emboss above
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawFittedText (b.getButtonText(), textArea.translated (0, 1),
                          juce::Justification::centred, 1);
        // Dark text fill
        g.setColour (juce::Colour (0xFF101012));
        g.drawFittedText (b.getButtonText(), textArea, juce::Justification::centred, 1);
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
                                            juce::ComboBox& /*cb*/)
    {
        const auto box = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);

        // Recessed dark face (deeper than the panel — like a milled window)
        juce::ColourGradient grad (
            juce::Colour (0xFF050507), box.getCentreX(), box.getY(),
            juce::Colour (0xFF1E1E22), box.getCentreX(), box.getBottom(), false);
        grad.addColour (0.40, juce::Colour (0xFF101015));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (box, 2.5f);

        // Real Gaussian inner shadow — true recessed look
        {
            juce::Path boxPath;
            boxPath.addRoundedRectangle (box, 2.5f);
            static thread_local melatonin::InnerShadow comboInner {
                { juce::Colours::black.withAlpha (0.85f), 5, { 0, 2 }, 0 }
            };
            comboInner.render (g, boxPath);
        }

        // Bottom edge highlight (catches light)
        g.setColour (juce::Colour (0xFF6A6A72).withAlpha (0.55f));
        g.drawLine (box.getX() + 3, box.getBottom() - 0.7f,
                    box.getRight() - 3, box.getBottom() - 0.7f, 0.6f);

        // Crisp outline
        g.setColour (juce::Colour (0xFF000000).withAlpha (0.85f));
        g.drawRoundedRectangle (box, 2.5f, 0.9f);

        // Hairline separator before arrow column
        g.setColour (juce::Colour (0xFF383840).withAlpha (0.65f));
        g.drawLine ((float) buttonX, box.getY() + 3,
                    (float) buttonX, box.getBottom() - 3, 0.7f);

        // White down-arrow
        const float ax = (float) (buttonX + buttonW * 0.5f);
        const float ay = (float) (buttonY + buttonH * 0.5f);
        juce::Path arrow;
        arrow.startNewSubPath (ax - 4.0f, ay - 2.0f);
        arrow.lineTo          (ax + 4.0f, ay - 2.0f);
        arrow.lineTo          (ax,        ay + 3.0f);
        arrow.closeSubPath();
        g.setColour (juce::Colour (0xFFE0E0E4));
        g.fillPath (arrow);
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

    /** Rich teak/walnut end-caps with photoreal grain. */
    void InstructionCardLnF::drawWoodEndCap (juce::Graphics& g, juce::Rectangle<int> r, bool isLeft)
    {
        const auto bf = r.toFloat();

        // ---- Warm base gradient (rich Beocord teak) ----
        const auto warm = juce::Colour (0xFFB87444);   // rich teak amber
        const auto mid  = juce::Colour (0xFF7A4022);   // mid teak
        const auto deep = juce::Colour (0xFF3A1808);   // dark walnut
        juce::ColourGradient grad (
            isLeft ? warm : deep, bf.getX(), bf.getCentreY(),
            isLeft ? deep : warm, bf.getRight(), bf.getCentreY(), false);
        grad.addColour (0.45, mid);
        g.setGradientFill (grad);
        g.fillRect (bf);

        // ---- Vertical grain streaks (long, irregular) ----
        juce::Random rng (isLeft ? 11111 : 22222);
        for (int i = 0; i < 26; ++i)
        {
            const float fx    = bf.getX() + rng.nextFloat() * bf.getWidth();
            const float alpha = rng.nextFloat() * 0.32f + 0.04f;
            const bool dark   = rng.nextBool();
            const float end   = fx + (rng.nextFloat() * 3.0f - 1.5f);
            g.setColour ((dark ? deep : warm).withAlpha (alpha));
            const float thickness = rng.nextFloat() * 0.6f + 0.25f;
            g.drawLine (fx, bf.getY(), end, bf.getBottom(), thickness);
        }

        // ---- Cathedral grain rings (the warm flame pattern of teak) ----
        for (int i = 0; i < 4; ++i)
        {
            const float cx = bf.getX() + rng.nextFloat() * bf.getWidth();
            const float cy = bf.getY() + rng.nextFloat() * bf.getHeight();
            const float rad = rng.nextFloat() * 50.0f + 30.0f;
            g.setColour (deep.withAlpha (0.10f));
            g.drawEllipse (cx - rad, cy - rad * 0.4f, rad * 2, rad * 0.8f, 0.7f);
        }

        // ---- Sparse pore-flecks (small dark pin-points) ----
        for (int i = 0; i < 30; ++i)
        {
            const float fx = bf.getX() + rng.nextFloat() * bf.getWidth();
            const float fy = bf.getY() + rng.nextFloat() * bf.getHeight();
            g.setColour (deep.withAlpha (rng.nextFloat() * 0.35f + 0.10f));
            g.fillEllipse (fx, fy, 0.8f, 0.8f);
        }

        // ---- Lacquer top sheen (subtle horizontal highlight at top edge) ----
        juce::ColourGradient lacquer (
            juce::Colour (0xFFFFE0B8).withAlpha (0.20f), bf.getCentreX(), bf.getY(),
            juce::Colours::transparentWhite, bf.getCentreX(), bf.getY() + 14, false);
        g.setGradientFill (lacquer);
        g.fillRect (bf.withHeight (16));

        // ---- Edge highlight on the INNER side (where teak meets the deck) ----
        const float edgeX = isLeft ? bf.getRight() - 2.0f : bf.getX() + 2.0f;
        juce::ColourGradient shine (
            juce::Colour (0xFFE8B080).withAlpha (0.55f), edgeX, bf.getY(),
            juce::Colours::transparentBlack, edgeX, bf.getY() + 60, false);
        g.setGradientFill (shine);
        g.fillRect (isLeft ? bf.withTrimmedLeft (bf.getWidth() - 5) : bf.withWidth (5));

        // ---- Outer edge shadow (3D thickness) ----
        const float outerX = isLeft ? bf.getX() : bf.getRight();
        juce::ColourGradient outerShadow (
            juce::Colours::black.withAlpha (0.50f), outerX, bf.getCentreY(),
            juce::Colours::transparentBlack,
            isLeft ? outerX + 5 : outerX - 5, bf.getCentreY(), false);
        g.setGradientFill (outerShadow);
        g.fillRect (isLeft ? bf.withWidth (5) : bf.withTrimmedLeft (bf.getWidth() - 5));
    }

    /** Black anodised aluminium control panel — Beocord 2400 / 2000 DL deck. */
    void InstructionCardLnF::drawBlackPanel (juce::Graphics& g, juce::Rectangle<int> r)
    {
        const auto bf = r.toFloat();

        // Deep matte black anodised gradient (slightly lifted at top)
        juce::ColourGradient grad (
            juce::Colour (0xFF1E1E20), bf.getCentreX(), bf.getY(),
            juce::Colour (0xFF050507), bf.getCentreX(), bf.getBottom(), false);
        grad.addColour (0.20, juce::Colour (0xFF14141A));
        grad.addColour (0.55, juce::Colour (0xFF0A0A10));
        g.setGradientFill (grad);
        g.fillRect (bf);

        // Horizontal brushed-anodise grain (seeded random → stable repaint)
        juce::Random rng (24681357);
        for (int i = 0; i < 160; ++i)
        {
            const float fy    = bf.getY() + rng.nextFloat() * bf.getHeight();
            const float alpha = rng.nextFloat() * 0.05f + 0.005f;
            const bool  hi    = rng.nextBool();
            g.setColour ((hi ? juce::Colour (0xFF50505A) : juce::Colours::black).withAlpha (alpha));
            const float thickness = rng.nextFloat() * 0.6f + 0.2f;
            g.drawLine (bf.getX(), fy, bf.getRight(), fy, thickness);
        }

        // Subtle radial vignette (centre slightly lifted by overhead light)
        juce::ColourGradient vig (
            juce::Colours::transparentBlack, bf.getCentreX(), bf.getCentreY(),
            juce::Colours::black.withAlpha (0.35f), bf.getX(), bf.getBottom(), true);
        g.setGradientFill (vig);
        g.fillRect (bf);

        // Top chrome separator (catches light at the deck/panel boundary)
        g.setColour (juce::Colour (0xFF8A8A92).withAlpha (0.85f));
        g.drawLine (bf.getX(), bf.getY() + 0.5f, bf.getRight(), bf.getY() + 0.5f, 0.7f);
        g.setColour (juce::Colour (0xFF50505A).withAlpha (0.7f));
        g.drawLine (bf.getX(), bf.getY() + 1.7f, bf.getRight(), bf.getY() + 1.7f, 0.5f);
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
                                         float rotationRad, bool active,
                                         float tapeFillRatio, float motionAmount)
    {
        // AUTHENTIC PRO REEL — brushed aluminium flange, dark tape visible
        // through 3 spoke windows, black hub with chrome screws. No bright
        // colours — the way a real Studer/Revox/Otari spool actually looks.
        const auto bf = r.toFloat();
        const float cx = bf.getCentreX();
        const float cy = bf.getCentreY();
        const float radius = juce::jmin (bf.getWidth(), bf.getHeight()) * 0.5f - 3.0f;

        // ---- Real Gaussian drop-shadow (melatonin) ----
        {
            juce::Path reelPath;
            reelPath.addEllipse (cx - radius, cy - radius, radius * 2, radius * 2);
            static thread_local melatonin::DropShadow reelShadow {
                { juce::Colours::black.withAlpha (0.75f), 18, { 0, 6 }, 0 }
            };
            reelShadow.render (g, reelPath);
        }

        // ---- 1. Outer dark rim (the metal edge of the flange) ----
        g.setColour (juce::Colour (0xFF1A1A1C));
        g.fillEllipse (cx - radius, cy - radius, radius * 2, radius * 2);

        // ---- 2. Brushed aluminium FLANGE — single solid metallic disc ----
        const float flangeR = radius * 0.96f;
        juce::ColourGradient flangeGrad (
            juce::Colour (0xFFD8D8D4), cx - flangeR * 0.45f, cy - flangeR * 0.45f,
            juce::Colour (0xFF6E6E6A), cx + flangeR * 0.45f, cy + flangeR * 0.45f, false);
        flangeGrad.addColour (0.30, juce::Colour (0xFFBABAB6));
        flangeGrad.addColour (0.65, juce::Colour (0xFF8E8E8A));
        g.setGradientFill (flangeGrad);
        g.fillEllipse (cx - flangeR, cy - flangeR, flangeR * 2, flangeR * 2);

        // Concentric brush-grain rings (rotate with reel for subtle motion)
        g.setColour (juce::Colour (0xFFFFFFFF).withAlpha (0.05f));
        for (float rr = flangeR * 0.94f; rr > flangeR * 0.45f; rr -= flangeR * 0.025f)
            g.drawEllipse (cx - rr, cy - rr, rr * 2, rr * 2, 0.4f);
        g.setColour (juce::Colour (0xFF000000).withAlpha (0.06f));
        for (float rr = flangeR * 0.93f; rr > flangeR * 0.45f; rr -= flangeR * 0.025f)
            g.drawEllipse (cx - rr, cy - rr, rr * 2, rr * 2, 0.3f);

        // ---- 3. WOUND-TAPE DISC — dynamic radius based on tapeFillRatio ----
        // This is the magnetic tape spooled on the hub. Supply reel shrinks
        // as audio plays back; takeup reel grows. Real Studer A800 behaviour.
        const float maxTapeR = flangeR * 0.92f;
        const float minTapeR = radius * 0.32f;        // empty hub diameter
        const float tapeOuterR = juce::jmap (juce::jlimit (0.0f, 1.0f, tapeFillRatio),
                                              minTapeR, maxTapeR);

        if (tapeFillRatio > 0.001f)
        {
            juce::ColourGradient tapeGrad (
                juce::Colour (0xFF2A2620), cx - tapeOuterR * 0.4f, cy - tapeOuterR * 0.4f,
                juce::Colour (0xFF080604), cx + tapeOuterR * 0.4f, cy + tapeOuterR * 0.4f, false);
            tapeGrad.addColour (0.5, juce::Colour (0xFF181410));
            g.setGradientFill (tapeGrad);
            g.fillEllipse (cx - tapeOuterR, cy - tapeOuterR, tapeOuterR * 2, tapeOuterR * 2);

            // Concentric tape-wind rings (sells the spooled-tape texture)
            g.setColour (juce::Colour (0xFF3E342A).withAlpha (0.40f));
            const float ringStep = juce::jmax (1.5f, tapeOuterR * 0.04f);
            for (float rr = tapeOuterR * 0.97f; rr > minTapeR * 0.95f; rr -= ringStep)
                g.drawEllipse (cx - rr, cy - rr, rr * 2, rr * 2, 0.4f);

            // Soft outer-edge highlight on the tape (catches light)
            g.setColour (juce::Colour (0xFF504030).withAlpha (0.50f));
            g.drawEllipse (cx - tapeOuterR, cy - tapeOuterR, tapeOuterR * 2, tapeOuterR * 2, 0.7f);
        }

        // ---- 4. THREE SPOKE WINDOWS — with optional MOTION BLUR ----
        const float spokeOuterR = flangeR * 0.92f;
        const float spokeInnerR = juce::jmax (radius * 0.30f, tapeOuterR + 1.0f);

        // Motion-blur: draw spokes at multiple angular offsets when reel
        // spins fast. Each pass at reduced alpha — composite gives a smeared
        // look identical to long-shutter photography.
        const int blurSteps = motionAmount > 0.05f ? 4 : 1;
        const float blurSpan = motionAmount * 0.13f;          // radians
        for (int s = 0; s < blurSteps; ++s)
        {
            const float t = (blurSteps == 1) ? 0.0f
                                              : (float) s / (float) (blurSteps - 1) - 0.5f;
            const float angOffset = t * blurSpan;
            const float pathAlpha = (blurSteps == 1) ? 1.0f
                                                      : 1.0f / (float) blurSteps + 0.10f;

            for (int i = 0; i < 3; ++i)
            {
                const float a = rotationRad + angOffset
                                + (float) i * juce::MathConstants<float>::twoPi / 3.0f;
                const float halfW = juce::MathConstants<float>::pi * 0.13f;

                juce::Path spoke;
                spoke.addPieSegment (cx - spokeOuterR, cy - spokeOuterR,
                                      spokeOuterR * 2, spokeOuterR * 2,
                                      a - halfW, a + halfW, spokeInnerR / spokeOuterR);

                juce::ColourGradient sg (
                    juce::Colour (0xFF1E1A14).withAlpha (pathAlpha),
                    cx, cy - spokeOuterR,
                    juce::Colour (0xFF050402).withAlpha (pathAlpha),
                    cx, cy + spokeOuterR, false);
                g.setGradientFill (sg);
                g.fillPath (spoke);

                // Edge highlights on the sharp (final) layer only
                if (s == blurSteps - 1)
                {
                    g.setColour (juce::Colours::black.withAlpha (0.55f));
                    g.strokePath (spoke, juce::PathStrokeType (0.8f));

                    juce::Path edgeHi;
                    edgeHi.addCentredArc (cx, cy, spokeOuterR, spokeOuterR, 0,
                                           a - halfW, a + halfW, true);
                    g.setColour (juce::Colour (0xFFEFEFE8).withAlpha (0.35f));
                    g.strokePath (edgeHi, juce::PathStrokeType (0.6f));
                }
            }
        }

        // ---- 5. Black hub ring (separates hub from tape) ----
        const float hubOuterR = radius * 0.30f;
        juce::ColourGradient hubRingGrad (
            juce::Colour (0xFF1E1E20), cx, cy - hubOuterR,
            juce::Colour (0xFF050507), cx, cy + hubOuterR, false);
        g.setGradientFill (hubRingGrad);
        g.fillEllipse (cx - hubOuterR, cy - hubOuterR, hubOuterR * 2, hubOuterR * 2);

        // ---- 6. Brushed aluminium central hub ----
        const float hubR = radius * 0.22f;
        juce::ColourGradient hubGrad (
            juce::Colour (0xFFE0E0DC), cx - hubR * 0.4f, cy - hubR * 0.5f,
            juce::Colour (0xFF6A6A66), cx + hubR * 0.4f, cy + hubR * 0.5f, false);
        hubGrad.addColour (0.5, juce::Colour (0xFFA8A8A4));
        g.setGradientFill (hubGrad);
        g.fillEllipse (cx - hubR, cy - hubR, hubR * 2, hubR * 2);
        g.setColour (juce::Colour (0xFF101012));
        g.drawEllipse (cx - hubR, cy - hubR, hubR * 2, hubR * 2, 0.9f);

        // ---- 7. Three Phillips screws on the hub (rotate with reel) ----
        const float screwR = juce::jmax (1.4f, hubR * 0.20f);
        const float screwOrbit = hubR * 0.55f;
        for (int i = 0; i < 3; ++i)
        {
            const float a  = rotationRad + (float) i * juce::MathConstants<float>::twoPi / 3.0f
                              + juce::MathConstants<float>::pi / 6.0f;
            const float sx = cx + screwOrbit * std::sin (a);
            const float sy = cy - screwOrbit * std::cos (a);

            juce::ColourGradient sGrad (
                juce::Colour (0xFFE0E0DC), sx - screwR * 0.5f, sy - screwR * 0.5f,
                juce::Colour (0xFF404042), sx + screwR * 0.5f, sy + screwR * 0.5f, false);
            g.setGradientFill (sGrad);
            g.fillEllipse (sx - screwR, sy - screwR, screwR * 2, screwR * 2);
            g.setColour (juce::Colour (0xFF101012));
            g.drawEllipse (sx - screwR, sy - screwR, screwR * 2, screwR * 2, 0.4f);
            // Phillips slot (cross)
            g.drawLine (sx - screwR * 0.65f, sy, sx + screwR * 0.65f, sy, 0.7f);
            g.drawLine (sx, sy - screwR * 0.65f, sx, sy + screwR * 0.65f, 0.7f);
        }

        // ---- 8. Centre drive-pin hole ----
        const float pinR = hubR * 0.16f;
        g.setColour (juce::Colour (0xFF080808));
        g.fillEllipse (cx - pinR, cy - pinR, pinR * 2, pinR * 2);

        // ---- 9. Concentric separator outlines (subtle depth cues) ----
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawEllipse (cx - flangeR, cy - flangeR, flangeR * 2, flangeR * 2, 0.7f);
        g.drawEllipse (cx - hubOuterR, cy - hubOuterR, hubOuterR * 2, hubOuterR * 2, 0.6f);

        // ---- 10. Specular highlight (top-left arc, sells the metal curvature) ----
        juce::Path glint;
        glint.addCentredArc (cx, cy, radius * 0.94f, radius * 0.94f, 0,
                              -juce::MathConstants<float>::pi * 0.55f,
                              -juce::MathConstants<float>::pi * 0.18f, true);
        g.setColour (juce::Colours::white.withAlpha (0.22f));
        g.strokePath (glint, juce::PathStrokeType (2.2f));
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
        // PREMIUM TRANSPORT KEY: brushed-aluminium face with bevelled edges,
        // multi-stop drop shadow, glass-like top sheen. Active state fills
        // with accent gradient + glow halo.
        const auto bf = r.toFloat().reduced (1.0f);
        const auto inkCol = juce::Colour (0xFF181408);
        const float keyOffset = down ? 1.5f : 0.0f;
        const auto keyRect = bf.translated (0, keyOffset);

        // ---- Real Gaussian drop-shadow (melatonin) ----
        if (! down)
        {
            juce::Path keyPath;
            keyPath.addRoundedRectangle (bf, 3.0f);
            static thread_local melatonin::DropShadow keyShadow {
                { juce::Colours::black.withAlpha (0.65f), 7, { 0, 3 }, 0 }
            };
            keyShadow.render (g, keyPath);
        }

        // ---- Body fill ----
        if (active)
        {
            // Real Gaussian glow halo (melatonin)
            juce::Path keyPath;
            keyPath.addRoundedRectangle (keyRect, 3.0f);
            static thread_local melatonin::DropShadow keyGlow {
                { juce::Colour (0xFFE03828).withAlpha (0.55f), 14, { 0, 0 }, 2 }
            };
            keyGlow.render (g, keyPath);

            juce::ColourGradient g1 (
                accent.brighter (0.30f), keyRect.getCentreX(), keyRect.getY(),
                accent.darker (0.30f),   keyRect.getCentreX(), keyRect.getBottom(), false);
            g1.addColour (0.4, accent.brighter (0.05f));
            g.setGradientFill (g1);
            g.fillRoundedRectangle (keyRect, 3.0f);

            // Glass sheen on active state
            juce::ColourGradient sheen (
                juce::Colours::white.withAlpha (0.35f), keyRect.getCentreX(), keyRect.getY(),
                juce::Colours::transparentWhite,         keyRect.getCentreX(),
                keyRect.getY() + keyRect.getHeight() * 0.55f, false);
            g.setGradientFill (sheen);
            g.fillRoundedRectangle (keyRect.reduced (1.0f, 0.5f), 2.5f);
        }
        else
        {
            // White plastic key (Beocord 2400 style)
            juce::ColourGradient g1 (
                juce::Colour (0xFFEEEEEE), keyRect.getCentreX(), keyRect.getY(),
                juce::Colour (0xFFB0B0B2), keyRect.getCentreX(), keyRect.getBottom(), false);
            g1.addColour (0.30, juce::Colour (0xFFE0E0E0));
            g1.addColour (0.65, juce::Colour (0xFFCACACA));
            g.setGradientFill (g1);
            g.fillRoundedRectangle (keyRect, 3.0f);
        }

        // ---- Top hairline highlight ----
        if (! down)
        {
            g.setColour (juce::Colour (active ? 0xFFF2F2EC : 0xFFFFFFFF).withAlpha (active ? 0.85f : 0.85f));
            g.drawLine (keyRect.getX() + 3, keyRect.getY() + 0.7f,
                        keyRect.getRight() - 3, keyRect.getY() + 0.7f, 0.7f);
        }

        // ---- Bottom shadow line ----
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawLine (keyRect.getX() + 3, keyRect.getBottom() - 0.7f,
                    keyRect.getRight() - 3, keyRect.getBottom() - 0.7f, 0.7f);

        // ---- Crisp dark outline ----
        g.setColour (juce::Colour (0xFF080808).withAlpha (0.85f));
        g.drawRoundedRectangle (keyRect, 3.0f, 0.9f);
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

        // ===================================================================
        //  PREMIUM BEZEL — heavy black anodised housing with chrome trim
        // ===================================================================

        // Outer drop shadow (real Gaussian via melatonin)
        {
            juce::Path bezelPath;
            bezelPath.addRoundedRectangle (bf, 5.0f);
            static thread_local melatonin::DropShadow vuOuterShadow {
                { juce::Colours::black.withAlpha (0.70f), 12, { 0, 5 }, 1 }
            };
            vuOuterShadow.render (g, bezelPath);
        }

        // Heavy black bezel — multi-stop gradient for depth
        juce::ColourGradient bezelGrad (
            juce::Colour (0xFF3A3A3E), bf.getCentreX(), bf.getY(),
            juce::Colour (0xFF030305), bf.getCentreX(), bf.getBottom(), false);
        bezelGrad.addColour (0.20, juce::Colour (0xFF2A2A2E));
        bezelGrad.addColour (0.55, juce::Colour (0xFF15151A));
        g.setGradientFill (bezelGrad);
        g.fillRoundedRectangle (bf, 5.0f);

        // Bright top hairline (catches light at the bezel edge)
        g.setColour (juce::Colour (0xFF8A8A92).withAlpha (0.9f));
        g.drawLine (bf.getX() + 4, bf.getY() + 0.8f,
                    bf.getRight() - 4, bf.getY() + 0.8f, 0.8f);

        // Outer chrome trim line
        g.setColour (juce::Colour (0xFFB0B0B6).withAlpha (0.55f));
        g.drawRoundedRectangle (bf.reduced (0.4f), 5.0f, 0.7f);

        // Four chrome corner screws (premium meter detail)
        const float screwR = juce::jmax (1.8f, juce::jmin (bf.getWidth(), bf.getHeight()) * 0.018f);
        const float screwInset = screwR + 4.0f;
        for (auto p : { juce::Point<float> (bf.getX() + screwInset, bf.getY() + screwInset),
                        juce::Point<float> (bf.getRight() - screwInset, bf.getY() + screwInset),
                        juce::Point<float> (bf.getX() + screwInset, bf.getBottom() - screwInset),
                        juce::Point<float> (bf.getRight() - screwInset, bf.getBottom() - screwInset) })
        {
            juce::ColourGradient sg (
                juce::Colour (0xFFD8D8DA), p.x - screwR * 0.5f, p.y - screwR * 0.5f,
                juce::Colour (0xFF40404A), p.x + screwR * 0.5f, p.y + screwR * 0.5f, false);
            g.setGradientFill (sg);
            g.fillEllipse (p.x - screwR, p.y - screwR, screwR * 2, screwR * 2);
            g.setColour (juce::Colour (0xFF050507));
            g.drawEllipse (p.x - screwR, p.y - screwR, screwR * 2, screwR * 2, 0.5f);
            // Slot
            g.drawLine (p.x - screwR * 0.6f, p.y, p.x + screwR * 0.6f, p.y, 0.7f);
        }

        // ---- BACKLIT AMBER METER FACE (Fairchild 670 reference) ----
        const float screwBuffer = screwR * 2.0f + 6.0f;
        const auto face = bf.reduced (screwBuffer, screwR + 5.0f);

        // Recessed shadow ring
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.fillRoundedRectangle (face.expanded (1.4f), 3.5f);

        // Deep amber/gold base (the iconic Fairchild backlit warmth)
        juce::ColourGradient faceGrad (
            juce::Colour (0xFFE8C078), face.getCentreX(), face.getY(),
            juce::Colour (0xFFB87830), face.getCentreX(), face.getBottom(), false);
        faceGrad.addColour (0.45, juce::Colour (0xFFD8A858));
        g.setGradientFill (faceGrad);
        g.fillRoundedRectangle (face, 3.0f);

        // Strong radial backlit glow from below — sells the lamp-behind-dial
        {
            juce::ColourGradient lamp (
                juce::Colour (0xFFFFE0A0).withAlpha (0.85f),
                face.getCentreX(), face.getCentreY() + face.getHeight() * 0.6f,
                juce::Colours::transparentWhite,
                face.getCentreX(), face.getY(), true);
            g.setGradientFill (lamp);
            g.fillRoundedRectangle (face, 3.0f);
        }
        // Real Gaussian glow inside (melatonin) — gives true backlit hot-spot
        {
            juce::Path facePath;
            facePath.addRoundedRectangle (face, 3.0f);
            static thread_local melatonin::DropShadow faceBacklight {
                { juce::Colour (0xFFFFB060).withAlpha (0.30f), 12, { 0, 4 }, 0 }
            };
            // (Drop-shadow on the face from inside? Use as halo by drawing path
            // smaller and using shadow as the amber bloom.)
            const auto inner = face.reduced (2.0f);
            juce::Path innerPath;
            innerPath.addRoundedRectangle (inner, 2.5f);
            faceBacklight.render (g, innerPath);
        }

        // Face inner shadow at top (real Gaussian, gives true recess feel)
        {
            juce::Path facePath;
            facePath.addRoundedRectangle (face, 3.0f);
            static thread_local melatonin::InnerShadow vuInner {
                { juce::Colours::black.withAlpha (0.65f), 8, { 0, 3 }, 0 }
            };
            vuInner.render (g, facePath);
        }

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

        // VU mark (the canonical printed "VU" wordmark on the dial)
        g.setColour (inkCol);
        g.setFont (logoFont (11.0f));
        g.drawText ("VU", face.toNearestInt().withTrimmedTop ((int) (face.getHeight() * 0.62f)),
                     juce::Justification::centred, false);
        // Manufacturer line under VU
        g.setFont (sectionFont (6.5f));
        g.setColour (inkCol.withAlpha (0.55f));
        g.drawText ("SOUNDBOYS", face.toNearestInt()
                       .withTrimmedTop ((int) (face.getHeight() * 0.80f)),
                     juce::Justification::centred, false);
        juce::ignoreUnused (channel);

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

        // ===================================================================
        //  PREMIUM GLASS COVER — thick convex glass, multi-layer reflections,
        //  chromatic edge, dust speck, subtle tint. Studio-quality finish.
        // ===================================================================

        // 1. Cool tint cast (premium tinted protective glass)
        g.setColour (juce::Colour (0xFFA8C8E0).withAlpha (0.08f));
        g.fillRoundedRectangle (face, 3.0f);

        // 2. Convex bottom darkening (light falls off at lower curve)
        juce::ColourGradient glassDarken (
            juce::Colours::transparentBlack, face.getCentreX(), face.getCentreY() - face.getHeight() * 0.10f,
            juce::Colours::black.withAlpha (0.28f), face.getCentreX(), face.getBottom(), false);
        g.setGradientFill (glassDarken);
        g.fillRoundedRectangle (face, 3.0f);

        // 3. PRIMARY specular sweep — big diagonal across upper half
        {
            juce::Path sweep;
            sweep.startNewSubPath (face.getX() - 2, face.getY() - 1);
            sweep.lineTo          (face.getX() + face.getWidth() * 0.92f, face.getY() - 1);
            sweep.lineTo          (face.getX() + face.getWidth() * 0.42f,
                                    face.getY() + face.getHeight() * 0.62f);
            sweep.lineTo          (face.getX() - 2,
                                    face.getY() + face.getHeight() * 0.45f);
            sweep.closeSubPath();

            juce::Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (sweep);
            juce::ColourGradient sg (
                juce::Colours::white.withAlpha (0.62f), face.getX(), face.getY(),
                juce::Colours::transparentWhite,         face.getX(),
                face.getY() + face.getHeight() * 0.62f, false);
            g.setGradientFill (sg);
            g.fillRoundedRectangle (face, 3.0f);
        }

        // 4. SECONDARY bright crescent highlight (the "wet" sheen at top)
        {
            juce::Path crescent;
            const float cy0 = face.getY() - face.getHeight() * 0.65f;
            const float cw  = face.getWidth() * 1.5f;
            const float ch  = face.getHeight() * 1.6f;
            const float cx0 = face.getCentreX() - cw * 0.5f;
            crescent.addEllipse (cx0, cy0, cw, ch);

            juce::Path mask;
            mask.addRoundedRectangle (face, 3.0f);
            crescent.setUsingNonZeroWinding (true);

            juce::Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (mask);
            g.reduceClipRegion (crescent);
            juce::ColourGradient cg (
                juce::Colours::white.withAlpha (0.55f), face.getCentreX(), face.getY() - 4,
                juce::Colours::transparentWhite,         face.getCentreX(),
                face.getY() + face.getHeight() * 0.4f, false);
            g.setGradientFill (cg);
            g.fillRect (face.expanded (4.0f));
        }

        // 5. Side reflection — vertical strip on right side (inner edge of glass)
        {
            const float sw = face.getWidth() * 0.05f;
            const auto strip = juce::Rectangle<float> (face.getRight() - sw - 2,
                                                        face.getY() + 2,
                                                        sw, face.getHeight() - 4);
            juce::ColourGradient sg (
                juce::Colours::transparentWhite, strip.getX(), strip.getCentreY(),
                juce::Colours::white.withAlpha (0.22f), strip.getRight(), strip.getCentreY(), false);
            g.setGradientFill (sg);
            g.fillRect (strip);
        }

        // 6. Lower-right small highlight blob (point-light reflection)
        {
            const float bw = face.getWidth() * 0.18f;
            const float bh = face.getHeight() * 0.12f;
            const float bx = face.getRight() - bw - face.getWidth() * 0.10f;
            const float by = face.getBottom() - bh - face.getHeight() * 0.18f;
            juce::ColourGradient bg (
                juce::Colours::white.withAlpha (0.28f), bx + bw * 0.5f, by + bh * 0.5f,
                juce::Colours::transparentWhite,         bx + bw, by + bh, true);
            g.setGradientFill (bg);
            g.fillEllipse (bx, by, bw, bh);
        }

        // 7. Chromatic rim — a faint coloured fringe at the bezel edge
        //    (the way real glass refracts light at its perimeter)
        g.setColour (juce::Colour (0xFFB8E0FF).withAlpha (0.30f));
        g.drawRoundedRectangle (face.reduced (0.4f), 3.0f, 0.6f);
        g.setColour (juce::Colour (0xFFFFD0A0).withAlpha (0.20f));
        g.drawRoundedRectangle (face.reduced (1.0f), 2.6f, 0.5f);

        // 8. Bright top edge — sharp white catch at the very top of the glass
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.drawLine (face.getX() + 4, face.getY() + 0.7f,
                    face.getRight() - 4, face.getY() + 0.7f, 0.9f);

        // 9. Inner deep shadow at bezel (sells thickness — glass sits IN metal)
        juce::ColourGradient innerShadow (
            juce::Colours::black.withAlpha (0.55f), face.getCentreX(), face.getY() - 1,
            juce::Colours::transparentBlack,         face.getCentreX(), face.getY() + 3, false);
        g.setGradientFill (innerShadow);
        g.drawRoundedRectangle (face, 3.0f, 1.5f);

        // 10. Final outline — crisp dark line where glass ends
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawRoundedRectangle (face, 3.0f, 0.7f);

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
