/*  BC2000LookAndFeel implementation. */

#include "BC2000LookAndFeel.h"

namespace bc2000dl::ui
{
    //=========================================================================
    //  Construction
    //=========================================================================
    InstructionCardLnF::InstructionCardLnF()
    {
        setColour (juce::ResizableWindow::backgroundColourId, paper());
        setColour (juce::DocumentWindow::backgroundColourId, paper());
        setColour (juce::Label::textColourId, ink());
        setColour (juce::TextEditor::backgroundColourId, paperDarker());
        setColour (juce::TextEditor::textColourId, ink());
        setColour (juce::TextEditor::outlineColourId, ink().withAlpha (0.4f));
        setColour (juce::ComboBox::backgroundColourId, paperDarker());
        setColour (juce::ComboBox::textColourId, ink());
        setColour (juce::ComboBox::outlineColourId, ink().withAlpha (0.5f));
        setColour (juce::ComboBox::arrowColourId, redAccent());
        setColour (juce::PopupMenu::backgroundColourId, paper());
        setColour (juce::PopupMenu::textColourId, ink());
        setColour (juce::PopupMenu::highlightedBackgroundColourId, redAccent());
        setColour (juce::PopupMenu::highlightedTextColourId, paper());
        setColour (juce::TextButton::buttonColourId, paperDarker());
        setColour (juce::TextButton::textColourOffId, ink());
        setColour (juce::TextButton::textColourOnId, paper());
        setColour (juce::TooltipWindow::backgroundColourId, ink());
        setColour (juce::TooltipWindow::textColourId, paper());
        setColour (juce::TooltipWindow::outlineColourId, ink());
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
        f.setExtraKerningFactor (0.10f);
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

    juce::Font InstructionCardLnF::getComboBoxFont   (juce::ComboBox&)        { return labelFont (11.0f); }
    juce::Font InstructionCardLnF::getLabelFont      (juce::Label&)           { return labelFont (11.0f); }
    juce::Font InstructionCardLnF::getTextButtonFont (juce::TextButton&, int h) { return labelFont ((float) juce::jmin (12, h - 8)); }
    juce::Font InstructionCardLnF::getPopupMenuFont()                          { return labelFont (12.0f); }

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
            g.setColour (ink().withAlpha (0.3f));
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
            g.setColour (textColour != nullptr ? *textColour : ink());
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
    //  Slider rendering — vertical slide-faders (B&O style)
    //=========================================================================
    void InstructionCardLnF::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                                                float sliderPos, float, float,
                                                juce::Slider::SliderStyle style,
                                                juce::Slider& s)
    {
        if (style != juce::Slider::LinearVertical)
        {
            // Fall back to default for non-vertical
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, 0, 0, style, s);
            return;
        }

        const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        const float capH = 18.0f;
        const float capW = juce::jmin ((float) w - 2.0f, 22.0f);
        const float channelW = 4.0f;

        // Channel — thin black slot
        const float chTop = area.getY() + capH * 0.5f;
        const float chBot = area.getBottom() - capH * 0.5f;
        const juce::Rectangle<float> channel (
            area.getCentreX() - channelW * 0.5f, chTop, channelW, chBot - chTop);
        drawSlideChannel (g, channel);

        // Cap position (sliderPos is in pixels, vertical)
        const float capY = sliderPos - capH * 0.5f;
        const juce::Rectangle<float> cap (area.getCentreX() - capW * 0.5f, capY, capW, capH);
        drawSlideFaderCap (g, cap, true);
    }

    void InstructionCardLnF::drawSlideChannel (juce::Graphics& g, juce::Rectangle<float> ch)
    {
        // Recessed dark slot with subtle inner shadow
        g.setColour (ink());
        g.fillRoundedRectangle (ch, 1.0f);
        g.setColour (ink().darker (0.4f));
        g.drawRoundedRectangle (ch, 1.0f, 0.8f);

        // Tick marks every 10% (11 ticks: 0..10)
        g.setColour (ink().withAlpha (0.45f));
        for (int i = 0; i <= 10; ++i)
        {
            const float ty = ch.getY() + (ch.getHeight() * (float) i / 10.0f);
            const float tw = (i % 5 == 0) ? 5.0f : 3.0f;
            g.drawLine (ch.getX() - tw, ty, ch.getX() - 1.0f, ty, 0.7f);
            g.drawLine (ch.getRight() + 1.0f, ty, ch.getRight() + tw, ty, 0.7f);
        }
    }

    void InstructionCardLnF::drawSlideFaderCap (juce::Graphics& g, juce::Rectangle<float> r,
                                                  bool bright)
    {
        // Silver gradient cap with red center line
        juce::ColourGradient grad (
            metalHi(),  r.getX(), r.getY() + r.getHeight() * 0.2f,
            metalLo(),  r.getX(), r.getBottom(), false);
        if (bright) grad.addColour (0.5, juce::Colour (0xffe5e5e3));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 2.0f);

        // Outline
        g.setColour (ink().withAlpha (0.5f));
        g.drawRoundedRectangle (r, 2.0f, 0.7f);

        // Red center line — the iconic Beocord red position-marker
        const float cy = r.getCentreY();
        g.setColour (redAccent());
        g.drawLine (r.getX() + 2.0f, cy, r.getRight() - 2.0f, cy, 1.6f);

        // Knurled top edge hint (two thin lines)
        g.setColour (ink().withAlpha (0.25f));
        g.drawLine (r.getX() + 3, cy - 4, r.getRight() - 3, cy - 4, 0.4f);
        g.drawLine (r.getX() + 3, cy + 4, r.getRight() - 3, cy + 4, 0.4f);
    }

    //=========================================================================
    //  Rotary slider
    //=========================================================================
    void InstructionCardLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                                float sliderPos,
                                                float angleStart, float angleEnd,
                                                juce::Slider& s)
    {
        const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h)
                              .reduced (3.0f);
        const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
        const float cx = area.getCentreX();
        const float cy = area.getCentreY();
        const float angle = angleStart + sliderPos * (angleEnd - angleStart);

        // Outer ring (chrome)
        juce::ColourGradient ringGrad (
            metalHi(), cx - radius * 0.5f, cy - radius * 0.5f,
            metalLo(), cx + radius * 0.5f, cy + radius * 0.5f, false);
        g.setGradientFill (ringGrad);
        g.fillEllipse (cx - radius, cy - radius, radius * 2, radius * 2);

        // Dark knob body
        const float bodyR = radius * 0.82f;
        juce::ColourGradient bodyGrad (
            juce::Colour (0xff3a3a36), cx, cy - bodyR,
            juce::Colour (0xff1a1a18), cx, cy + bodyR, false);
        g.setGradientFill (bodyGrad);
        g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2, bodyR * 2);

        // Body outline
        g.setColour (ink());
        g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2, bodyR * 2, 0.8f);

        // Indicator line (from center to outer edge)
        const float indR0 = bodyR * 0.30f;
        const float indR1 = bodyR * 0.85f;
        const float ix0 = cx + indR0 * std::sin (angle);
        const float iy0 = cy - indR0 * std::cos (angle);
        const float ix1 = cx + indR1 * std::sin (angle);
        const float iy1 = cy - indR1 * std::cos (angle);
        g.setColour (s.isEnabled() ? amberHot() : inkSoft());
        g.drawLine (ix0, iy0, ix1, iy1, 2.2f);

        // Center dot
        g.setColour (ink().darker (0.5f));
        g.fillEllipse (cx - 1.6f, cy - 1.6f, 3.2f, 3.2f);
    }

    //=========================================================================
    //  Toggle button — instruction-card style: small square with check or red light
    //=========================================================================
    void InstructionCardLnF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                                 bool highlighted, bool /*down*/)
    {
        auto bounds = b.getLocalBounds().toFloat().reduced (1.0f);
        const float boxW = juce::jmin (bounds.getHeight(), 18.0f);
        const auto box = juce::Rectangle<float> (bounds.getX(), bounds.getCentreY() - boxW * 0.5f,
                                                  boxW, boxW);

        // Box outline
        g.setColour (paperDarker());
        g.fillRoundedRectangle (box, 2.0f);
        g.setColour (ink());
        g.drawRoundedRectangle (box, 2.0f, 0.8f);

        // ON state — fill with red (instruction-card style for "active")
        if (b.getToggleState())
        {
            g.setColour (redAccent());
            g.fillRoundedRectangle (box.reduced (2.5f), 1.0f);
        }
        else if (highlighted)
        {
            g.setColour (paperShadow().withAlpha (0.6f));
            g.fillRoundedRectangle (box.reduced (2.5f), 1.0f);
        }

        // Label to the right
        bounds.removeFromLeft (boxW + 6);
        g.setColour (ink());
        g.setFont (labelFont (10.5f));
        g.drawFittedText (b.getButtonText(), bounds.toNearestInt(),
                           juce::Justification::centredLeft, 1);
    }

    //=========================================================================
    //  Button background — used for transport keys + preset nav
    //=========================================================================
    void InstructionCardLnF::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                                     const juce::Colour& /*bgColour*/,
                                                     bool highlighted, bool down)
    {
        const auto r = b.getLocalBounds().toFloat();
        // Treat any text-button as a transport piano-key style
        const bool active = b.getToggleState() || down;
        drawTransportKey (g, b.getLocalBounds(), down, active,
                           b.getName().contains ("REC") ? redAccent() : amber());
        juce::ignoreUnused (highlighted, r);
    }

    //=========================================================================
    //  Combo box — paper-darker fill with red arrow
    //=========================================================================
    void InstructionCardLnF::drawComboBox (juce::Graphics& g, int width, int height,
                                            bool /*isButtonDown*/,
                                            int buttonX, int buttonY,
                                            int buttonW, int buttonH,
                                            juce::ComboBox& cb)
    {
        const auto box = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);

        // Body
        g.setColour (paperDarker());
        g.fillRoundedRectangle (box, 2.0f);
        g.setColour (ink());
        g.drawRoundedRectangle (box, 2.0f, 0.7f);

        // Down-arrow on the right
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
    void InstructionCardLnF::drawPaperPanel (juce::Graphics& g, juce::Rectangle<int> r)
    {
        const auto bf = r.toFloat();

        // Subtle vertical gradient — paper is slightly lighter at top
        juce::ColourGradient grad (
            paper().brighter (0.04f), bf.getX(), bf.getY(),
            paperDarker().withAlpha (0.6f).overlaidWith (paper()),
            bf.getX(), bf.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRect (bf);

        // Edge shadow (paper-card depth illusion)
        juce::ColourGradient edge (
            ink().withAlpha (0.0f),  bf.getCentreX(), bf.getCentreY(),
            ink().withAlpha (0.18f), bf.getX(),       bf.getCentreY(), true);
        g.setGradientFill (edge);
        g.fillRect (bf);
    }

    void InstructionCardLnF::drawWoodEndCap (juce::Graphics& g, juce::Rectangle<int> r, bool isLeft)
    {
        const auto bf = r.toFloat();
        juce::ColourGradient grad (
            isLeft ? woodLight() : woodDark(),
            bf.getX(), bf.getCentreY(),
            isLeft ? woodDark() : woodLight(),
            bf.getRight(), bf.getCentreY(), false);
        g.setGradientFill (grad);
        g.fillRect (bf);

        // Wood grain (random thin vertical lines)
        juce::Random rng (12345);
        g.setColour (woodDark().withAlpha (0.4f));
        for (int i = 0; i < 8; ++i)
        {
            const float x = bf.getX() + rng.nextFloat() * bf.getWidth();
            g.drawLine (x, bf.getY(), x + rng.nextFloat() * 2.0f - 1.0f, bf.getBottom(), 0.4f);
        }
    }

    void InstructionCardLnF::drawSectionBox (juce::Graphics& g, juce::Rectangle<int> r,
                                              const juce::String& title)
    {
        const auto bf = r.toFloat();
        g.setColour (ink().withAlpha (0.45f));
        g.drawRect (bf, 0.7f);

        if (title.isNotEmpty())
        {
            // Tab in upper-left corner
            const auto tab = juce::Rectangle<float> (
                bf.getX() + 6, bf.getY() - 6, juce::jmax (40.0f, (float) title.length() * 6.0f), 12);
            g.setColour (paper());
            g.fillRect (tab);
            g.setColour (redAccent());
            g.setFont (sectionFont (8.5f));
            g.drawText (title, tab, juce::Justification::centred, false);
        }
    }

    void InstructionCardLnF::drawReel (juce::Graphics& g, juce::Rectangle<int> r,
                                         float rotationRad, bool active)
    {
        const auto bf = r.toFloat();
        const float cx = bf.getCentreX();
        const float cy = bf.getCentreY();
        const float radius = juce::jmin (bf.getWidth(), bf.getHeight()) * 0.5f - 2.0f;

        // Outer rim (tape edge)
        juce::ColourGradient rimGrad (
            ink().brighter (0.3f), cx - radius * 0.4f, cy - radius * 0.4f,
            ink(),                  cx + radius * 0.5f, cy + radius * 0.5f, false);
        g.setGradientFill (rimGrad);
        g.fillEllipse (cx - radius, cy - radius, radius * 2, radius * 2);

        // Tape pack (inner area where the magnetic tape is wound)
        const float tapeR = radius * 0.92f;
        juce::ColourGradient tapeGrad (
            juce::Colour (0xff2a1a10), cx, cy - tapeR,
            juce::Colour (0xff100806), cx, cy + tapeR, false);
        g.setGradientFill (tapeGrad);
        g.fillEllipse (cx - tapeR, cy - tapeR, tapeR * 2, tapeR * 2);

        // Subtle rotation lines (tape winding pattern) — only if rotating
        if (active)
        {
            g.setColour (juce::Colours::white.withAlpha (0.06f));
            for (int i = 0; i < 12; ++i)
            {
                const float a = rotationRad + (float) i * juce::MathConstants<float>::pi / 6.0f;
                const float r1 = tapeR * 0.60f;
                const float r2 = tapeR * 0.95f;
                const float x1 = cx + r1 * std::sin (a);
                const float y1 = cy - r1 * std::cos (a);
                const float x2 = cx + r2 * std::sin (a);
                const float y2 = cy - r2 * std::cos (a);
                g.drawLine (x1, y1, x2, y2, 0.6f);
            }
        }

        // Hub (center) — silver three-spoke
        const float hubR = radius * 0.38f;
        juce::ColourGradient hubGrad (
            metalHi(), cx, cy - hubR,
            metalLo(), cx, cy + hubR, false);
        g.setGradientFill (hubGrad);
        g.fillEllipse (cx - hubR, cy - hubR, hubR * 2, hubR * 2);
        g.setColour (ink());
        g.drawEllipse (cx - hubR, cy - hubR, hubR * 2, hubR * 2, 0.8f);

        // Three spokes (radial, at 0°, 120°, 240°)
        for (int i = 0; i < 3; ++i)
        {
            const float a = rotationRad + (float) i * juce::MathConstants<float>::pi * 2.0f / 3.0f;
            juce::Path spoke;
            const float sw = hubR * 0.18f;
            spoke.startNewSubPath (cx - sw, cy);
            spoke.lineTo          (cx + sw, cy);
            spoke.lineTo          (cx + sw * 0.4f, cy - hubR * 0.85f);
            spoke.lineTo          (cx - sw * 0.4f, cy - hubR * 0.85f);
            spoke.closeSubPath();
            juce::AffineTransform t = juce::AffineTransform::rotation (a, cx, cy);
            spoke.applyTransform (t);
            juce::ColourGradient spokeGrad (
                metalLo(), cx, cy,
                metalHi(), cx, cy - hubR, false);
            g.setGradientFill (spokeGrad);
            g.fillPath (spoke);
            g.setColour (ink().withAlpha (0.5f));
            g.strokePath (spoke, juce::PathStrokeType (0.4f));
        }

        // Center pin
        g.setColour (ink());
        g.fillEllipse (cx - 2.0f, cy - 2.0f, 4.0f, 4.0f);
    }

    void InstructionCardLnF::drawHeadAssembly (juce::Graphics& g, juce::Rectangle<int> r)
    {
        const auto bf = r.toFloat().reduced (4.0f, 8.0f);

        // Three head-blocks: erase, record, play (left to right)
        const float w = bf.getWidth() / 3.0f;
        const juce::String labels[] = { "E", "R", "P" };
        for (int i = 0; i < 3; ++i)
        {
            auto headRect = juce::Rectangle<float> (
                bf.getX() + i * w + 2, bf.getY(), w - 4, bf.getHeight());

            // Block body (silvery)
            juce::ColourGradient grad (
                metalHi(), headRect.getX(), headRect.getY(),
                metalLo(), headRect.getX(), headRect.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (headRect, 1.0f);
            g.setColour (ink().withAlpha (0.6f));
            g.drawRoundedRectangle (headRect, 1.0f, 0.6f);

            // Tape contact slot (vertical thin black line in the middle)
            g.setColour (ink());
            g.drawLine (headRect.getCentreX(), headRect.getY() + 2,
                        headRect.getCentreX(), headRect.getBottom() - 2, 1.0f);

            // Label
            g.setColour (ink());
            g.setFont (sectionFont (7.5f));
            g.drawText (labels[i],
                        headRect.removeFromBottom (10).toNearestInt(),
                        juce::Justification::centred, false);
        }

        // Capstan + pinch roller (small circle to right of heads)
        const float capR = bf.getHeight() * 0.18f;
        const float capX = bf.getRight() - capR * 1.5f;
        const float capY = bf.getCentreY();
        g.setColour (ink());
        g.fillEllipse (capX - capR, capY - capR, capR * 2, capR * 2);
        g.setColour (metalLo());
        g.fillEllipse (capX - capR * 0.5f, capY - capR * 0.5f, capR, capR);
    }

    void InstructionCardLnF::drawTransportKey (juce::Graphics& g, juce::Rectangle<int> r,
                                                  bool down, bool active, juce::Colour accent)
    {
        const auto bf = r.toFloat().reduced (1.0f);

        // Piano-key style: light cream top, darker bottom
        const float keyOffset = down ? 1.0f : 0.0f;
        const auto keyRect = bf.translated (0, keyOffset);

        juce::ColourGradient grad (
            paper().brighter (0.05f), keyRect.getX(), keyRect.getY(),
            paperDarker(),             keyRect.getX(), keyRect.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (keyRect, 2.5f);

        // Outline
        g.setColour (ink().withAlpha (0.55f));
        g.drawRoundedRectangle (keyRect, 2.5f, 0.7f);

        // Active indicator strip across top edge
        if (active)
        {
            const auto strip = juce::Rectangle<float> (
                keyRect.getX() + 4, keyRect.getY() + 2,
                keyRect.getWidth() - 8, 2.5f);
            g.setColour (accent);
            g.fillRoundedRectangle (strip, 1.0f);
        }

        // Drop shadow (when not pressed)
        if (! down)
        {
            g.setColour (ink().withAlpha (0.18f));
            g.drawLine (bf.getX() + 2, bf.getBottom() - 0.3f,
                        bf.getRight() - 2, bf.getBottom() - 0.3f, 0.7f);
        }
    }

    void InstructionCardLnF::drawArrowLabel (juce::Graphics& g, juce::Rectangle<int> r,
                                                const juce::String& text, bool arrowRight)
    {
        const auto bf = r.toFloat();
        g.setColour (ink());
        g.setFont (labelFont (10.0f));

        if (arrowRight)
        {
            const auto textArea = bf.withTrimmedRight (10).toNearestInt();
            g.drawText (text, textArea, juce::Justification::centredRight, false);
            // Arrow (small triangle pointing right)
            juce::Path arrow;
            const float ax = bf.getRight() - 4;
            const float ay = bf.getCentreY();
            arrow.startNewSubPath (ax - 5, ay - 3);
            arrow.lineTo          (ax,     ay);
            arrow.lineTo          (ax - 5, ay + 3);
            g.strokePath (arrow, juce::PathStrokeType (0.9f));
        }
        else
        {
            const auto textArea = bf.withTrimmedLeft (10).toNearestInt();
            g.drawText (text, textArea, juce::Justification::centredLeft, false);
            juce::Path arrow;
            const float ax = bf.getX() + 4;
            const float ay = bf.getCentreY();
            arrow.startNewSubPath (ax + 5, ay - 3);
            arrow.lineTo          (ax,     ay);
            arrow.lineTo          (ax + 5, ay + 3);
            g.strokePath (arrow, juce::PathStrokeType (0.9f));
        }
    }

    void InstructionCardLnF::drawTitle (juce::Graphics& g, juce::Rectangle<int> r,
                                          const juce::String& title,
                                          const juce::String& subtitle)
    {
        auto bf = r.toFloat();
        g.setColour (ink());
        g.setFont (logoFont (22.0f));
        g.drawText (title, bf.removeFromTop (28).toNearestInt(),
                     juce::Justification::left, false);

        if (subtitle.isNotEmpty())
        {
            g.setColour (redAccent());
            g.setFont (sectionFont (10.0f));
            g.drawText (subtitle, bf.toNearestInt(),
                         juce::Justification::topLeft, false);
        }
    }

    void InstructionCardLnF::drawCounter (juce::Graphics& g, juce::Rectangle<int> r,
                                            const juce::String& text)
    {
        const auto bf = r.toFloat();

        // Recessed dark window
        g.setColour (ink());
        g.fillRoundedRectangle (bf, 2.0f);
        g.setColour (ink().darker (0.5f));
        g.drawRoundedRectangle (bf, 2.0f, 0.7f);

        // Inner subtle metallic shine at top
        juce::ColourGradient shine (
            juce::Colour (0xff2a2a28).withAlpha (0.5f), bf.getX(), bf.getY(),
            juce::Colours::transparentBlack,             bf.getX(), bf.getY() + bf.getHeight() * 0.4f,
            false);
        g.setGradientFill (shine);
        g.fillRoundedRectangle (bf.reduced (1.0f), 1.0f);

        // Mechanical-style digits in soft amber
        g.setColour (amber());
        g.setFont (monoFont (juce::jmax (10.0f, bf.getHeight() * 0.6f)));
        g.drawText (text, bf.toNearestInt(),
                    juce::Justification::centred, false);
    }
}
