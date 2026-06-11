/*  WireframeEditor.cpp — implementation. See WireframeEditor.h for overview. */

#include "WireframeEditor.h"
#include "presets/Presets.h"
#include "BinaryData.h"

namespace
{
    // Cached photoreal images — shared across the editor (loaded once via ImageCache).
    juce::Image getWalnut()       { return juce::ImageCache::getFromMemory (BinaryData::walnut_bezel_png,         BinaryData::walnut_bezel_pngSize); }
    juce::Image getMattePanel()   { return juce::ImageCache::getFromMemory (BinaryData::matte_panel_png,          BinaryData::matte_panel_pngSize); }
    juce::Image getDarkStrip()    { return juce::ImageCache::getFromMemory (BinaryData::panel_dark_strip_png,     BinaryData::panel_dark_strip_pngSize); }
    juce::Image getReelLeft()     { return juce::ImageCache::getFromMemory (BinaryData::reel_left_png,            BinaryData::reel_left_pngSize); }
    juce::Image getReelRight()    { return juce::ImageCache::getFromMemory (BinaryData::reel_right_png,           BinaryData::reel_right_pngSize); }
    juce::Image getKnobFilm()     { return juce::ImageCache::getFromMemory (BinaryData::knob_chrome_filmstrip_png, BinaryData::knob_chrome_filmstrip_pngSize); }
    juce::Image getFaderThumb()   { return juce::ImageCache::getFromMemory (BinaryData::fader_thumb_chrome_png,   BinaryData::fader_thumb_chrome_pngSize); }
    juce::Image getKnobFilmBig()  { return juce::ImageCache::getFromMemory (BinaryData::knob_filmstrip_big_png,   BinaryData::knob_filmstrip_big_pngSize); }
}

namespace bc2000dl::ui
{
    using LnF = WireframeLookAndFeel;

    // Editor canvas storlek (matchar wireframe SVG 920×780)
    static constexpr int kW = 920;
    static constexpr int kH = 780;

    //==========================================================================
    //  WireframeLookAndFeel — photoreal: black panel + chrome controls
    //==========================================================================
    WireframeLookAndFeel::WireframeLookAndFeel()
    {
        setColour (juce::Slider::backgroundColourId,         juce::Colour (kPanelBlkLo));
        setColour (juce::Slider::thumbColourId,              juce::Colour (kChromeHi));
        setColour (juce::Slider::trackColourId,              juce::Colour (kSilkDim));
        setColour (juce::Slider::rotarySliderFillColourId,   juce::Colour (kChromeMid));
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (kStroke));
        setColour (juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId,  juce::Colours::transparentBlack);
        setColour (juce::ComboBox::backgroundColourId,       juce::Colour (kPanelBlk));
        setColour (juce::ComboBox::outlineColourId,          juce::Colour (kSilkDim));
        setColour (juce::ComboBox::textColourId,             juce::Colour (kSilk));
        setColour (juce::ComboBox::arrowColourId,            juce::Colour (kSilk));
        setColour (juce::PopupMenu::backgroundColourId,      juce::Colour (kPanelBlk));
        setColour (juce::PopupMenu::textColourId,            juce::Colour (kSilk));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (kAluLo));
        setColour (juce::Label::textColourId,                juce::Colour (kSilk));
        setColour (juce::TextButton::buttonColourId,         juce::Colour (kBtnFace));
        setColour (juce::TextButton::buttonOnColourId,       juce::Colour (kBtnShadow));
        setColour (juce::TextButton::textColourOffId,        juce::Colour (kStroke));
        setColour (juce::TextButton::textColourOnId,         juce::Colour (kStroke));
        setColour (juce::ToggleButton::textColourId,         juce::Colour (kStroke));
    }

    void WireframeLookAndFeel::drawLinearSlider (juce::Graphics& g,
                                                  int x, int y, int w, int h,
                                                  float sliderPos, float /*min*/, float /*max*/,
                                                  juce::Slider::SliderStyle /*style*/,
                                                  juce::Slider& s)
    {
        const auto cx = (float) x + (float) w * 0.5f;
        const auto trackTop = (float) y + 4.0f;
        const auto trackBot = (float) (y + h) - 4.0f;

        // ===== UAD-grade recessed slot (5 layers) =====
        // (a) Outer rim shadow (extra dark frame)
        g.setColour (juce::Colour (0xFF030303));
        g.fillRect (juce::Rectangle<float> (cx - 2.8f, trackTop - 0.6f, 5.6f, trackBot - trackTop + 1.2f));
        // (b) Slot well — horizontal gradient (dark left, lighter middle, dark right)
        juce::ColourGradient slotGrad (juce::Colour (0xFF080808), cx - 2.0f, trackTop,
                                         juce::Colour (0xFF1A1A1A), cx, trackTop, false);
        slotGrad.addColour (0.5, juce::Colour (0xFF2E2E2E));
        slotGrad.point2 = { cx + 2.0f, trackTop };
        g.setGradientFill (slotGrad);
        g.fillRect (juce::Rectangle<float> (cx - 2.0f, trackTop, 4.0f, trackBot - trackTop));
        // (c) Top opening dark cap
        g.setColour (juce::Colour (0xCC000000));
        g.fillRect (juce::Rectangle<float> (cx - 2.0f, trackTop, 4.0f, 1.0f));
        // (d) Right-edge bright chrome bevel
        g.setColour (juce::Colour (0x88FFFFFF));
        g.drawLine (cx + 1.9f, trackTop + 0.8f, cx + 1.9f, trackBot - 0.8f, 0.5f);
        // (e) Left-edge slight highlight (less than right — directional lighting)
        g.setColour (juce::Colour (0x22FFFFFF));
        g.drawLine (cx - 1.9f, trackTop + 0.8f, cx - 1.9f, trackBot - 0.8f, 0.3f);

        // ===== UAD-grade chrome fader cap (12 layers) =====
        const float capW = (float) juce::jmin (w + 6, 18);
        const float capH = 14.0f;
        const float capY = sliderPos - capH * 0.5f;
        const float capX = cx - capW * 0.5f;
        juce::Rectangle<float> cap { capX, capY, capW, capH };

        // (1) Multi-stack Gaussian-style drop shadow (4 layers for soft falloff)
        for (int i = 4; i >= 1; --i)
        {
            const float alpha = 0.12f / (float) i;
            g.setColour (juce::Colour::fromFloatRGBA (0, 0, 0, alpha));
            g.fillRoundedRectangle (juce::Rectangle<float> (capX - (float) i * 0.3f,
                                                              capY + 1.5f + (float) i * 0.4f,
                                                              capW + (float) i * 0.6f,
                                                              capH),
                                      2.5f);
        }

        // (2) Outer dark bevel ring
        g.setColour (juce::Colour (0xFF0E0E0E));
        g.fillRoundedRectangle (cap, 2.2f);

        // (3) Inner cap body — 7-stop polished chrome gradient
        juce::ColourGradient capGrad (juce::Colour (0xFFFAFAFA), capX, capY,
                                        juce::Colour (0xFF2A2A2A), capX, capY + capH, false);
        capGrad.addColour (0.12, juce::Colour (0xFFE4E4E4));
        capGrad.addColour (0.25, juce::Colour (0xFFC0C0C0));
        capGrad.addColour (0.42, juce::Colour (0xFF8C8C8C));
        capGrad.addColour (0.55, juce::Colour (0xFF5E5E5E));
        capGrad.addColour (0.70, juce::Colour (0xFF888888));
        capGrad.addColour (0.85, juce::Colour (0xFFA8A8A8));
        g.setGradientFill (capGrad);
        g.fillRoundedRectangle (cap.reduced (1.0f), 1.6f);

        // (4) Bright top reflection band (specular sheen)
        juce::ColourGradient topReflect (juce::Colour (0xFFFFFFFF), capX, capY + 1.0f,
                                            juce::Colour (0x00FFFFFF), capX, capY + 4.0f, false);
        g.setGradientFill (topReflect);
        g.fillRoundedRectangle (juce::Rectangle<float> (capX + 1.5f, capY + 1.0f, capW - 3.0f, 3.5f), 1.2f);

        // (5) Mid-cap dark band (where chrome meets shadow) — gives 3D feel
        g.setColour (juce::Colour (0x55000000));
        g.fillRect (juce::Rectangle<float> (capX + 1.5f, sliderPos - 0.8f, capW - 3.0f, 1.6f));
        g.setColour (juce::Colour (0xCC000000));
        g.fillRect (juce::Rectangle<float> (capX + 2.0f, sliderPos - 0.3f, capW - 4.0f, 0.6f));

        // (6) Center grip line — bright white slim accent
        g.setColour (juce::Colour (0xCCFFFFFF));
        g.drawLine (capX + 2.5f, sliderPos + 0.6f, capX + capW - 2.5f, sliderPos + 0.6f, 0.4f);

        // (7) Bottom shadow band (cast shadow on cap base)
        g.setColour (juce::Colour (0xAA000000));
        g.fillRect (juce::Rectangle<float> (capX + 1.5f, capY + capH - 2.5f, capW - 3.0f, 1.2f));

        // (8) Bottom secondary highlight (very subtle reflection from below)
        g.setColour (juce::Colour (0x44FFFFFF));
        g.drawLine (capX + 2.0f, capY + capH - 1.2f, capX + capW - 2.0f, capY + capH - 1.2f, 0.3f);

        // (9) Left-edge chrome highlight (directional lighting from top-left)
        g.setColour (juce::Colour (0xCCFFFFFF));
        g.drawLine (capX + 0.7f, capY + 2.5f, capX + 0.7f, capY + capH - 2.5f, 0.5f);

        // (10) Right-edge dark shadow
        g.setColour (juce::Colour (0x99000000));
        g.drawLine (capX + capW - 0.7f, capY + 2.5f, capX + capW - 0.7f, capY + capH - 2.5f, 0.5f);

        // (11) Corner highlights (4 small bright dots for molded-chrome feel)
        g.setColour (juce::Colour (0xDDFFFFFF));
        g.fillEllipse (capX + 1.0f, capY + 1.5f, 1.4f, 1.4f);
        g.fillEllipse (capX + capW - 2.4f, capY + 1.5f, 1.4f, 1.4f);

        // (12) Outer outline
        g.setColour (juce::Colour (kStroke));
        g.drawRoundedRectangle (cap, 2.2f, 0.6f);

        juce::ignoreUnused (s);
    }

    void WireframeLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                                  int x, int y, int w, int h,
                                                  float sliderPos,
                                                  float rotaryStartAngle,
                                                  float rotaryEndAngle,
                                                  juce::Slider& s)
    {
        // =====================================================================
        // VINTAGE TUBE-COMPRESSOR KNOB — procedural rendering.
        //
        // Filmstrip experiments (Blender v4-v5 with HDRI + shadow catcher) were
        // dropped: at our 22-44 px knob sizes the photoreal detail downsamples
        // away.  Procedural is optimised for these dimensions, faster paint,
        // smaller binary, and resolution-independent.
        //
        // See /tmp/knob_render/ for the abandoned Blender pipeline — it works,
        // but only adds value at ≥80 px knob diameters.
        // =====================================================================
        const auto bounds = juce::Rectangle<float> ((float) x, (float) y,
                                                     (float) w, (float) h).reduced (2.0f);
        const auto cx = bounds.getCentreX();
        const auto cy = bounds.getCentreY();
        const auto r  = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;

        const auto angle = rotaryStartAngle
                         + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const float sinA = std::sin (angle);
        const float cosA = std::cos (angle);

        const float bevelOuterR = r;
        const float bevelInnerR = r * 0.86f;
        const float bodyR       = r * 0.84f;
        const float insetR      = r * 0.40f;
        const float dotR        = r * 0.085f;
        const float dotOrbit    = r * 0.62f;

        // ----- (1) Surround tick marks — clean, no numerals -----
        // 9 small ticks distributed around the rotation arc.
        // Center tick (12 o'clock) is slightly longer for the visual detent.
        // Cream-white silkscreen color, no shadow → reads as clean printed marks.
        if (r >= 14.0f)
        {
            const float tickR0    = r * 1.10f;
            const float tickR1    = r * 1.18f;
            const float tickRCent = r * 1.22f;
            const int nTicks = 9;
            for (int i = 0; i < nTicks; ++i)
            {
                const float t  = (float) i / (float) (nTicks - 1);
                const float ta = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
                const float ts = std::sin (ta);
                const float tc = std::cos (ta);
                const bool isCenter = (i == nTicks / 2);
                const float r1 = isCenter ? tickRCent : tickR1;
                const float lw = isCenter ? 0.7f : 0.5f;
                g.setColour (juce::Colour (0xFFB8B2A0));    // muted cream
                g.drawLine (cx + ts * tickR0, cy - tc * tickR0,
                             cx + ts * r1,     cy - tc * r1, lw);
            }
        }

        // ----- (2) Drop shadow — CACHED melatonin Gaussian blur -----
        // Replaces previous 6-stack ellipse fill loop.  Cached blur is computed
        // once per unique knob size and reused on subsequent paints → ~10× faster
        // and produces a smoother, more physically-correct gradient.
        {
            juce::Path shadowPath;
            shadowPath.addEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
            knobDropShadow.render (g, shadowPath);
        }

        // ----- (3) OUTER BEVEL RING — thick metallic collar with brushed texture -----
        {
            juce::ColourGradient bevGr (juce::Colour (0xFF4A4A4A),
                                          cx, cy - bevelOuterR * 0.92f,
                                          juce::Colour (0xFF050505),
                                          cx, cy + bevelOuterR * 0.92f, false);
            bevGr.addColour (0.30, juce::Colour (0xFF2E2E2E));
            bevGr.addColour (0.55, juce::Colour (0xFF181818));
            bevGr.addColour (0.78, juce::Colour (0xFF0E0E0E));
            g.setGradientFill (bevGr);
            g.fillEllipse (cx - bevelOuterR, cy - bevelOuterR,
                            bevelOuterR * 2.0f, bevelOuterR * 2.0f);
        }
        // Brushed striations on bevel ring
        {
            const juce::Graphics::ScopedSaveState saved (g);
            juce::Path ringClip;
            ringClip.addEllipse (cx - bevelOuterR, cy - bevelOuterR,
                                  bevelOuterR * 2.0f, bevelOuterR * 2.0f);
            juce::Path innerHole;
            innerHole.addEllipse (cx - bevelInnerR, cy - bevelInnerR,
                                   bevelInnerR * 2.0f, bevelInnerR * 2.0f);
            ringClip.setUsingNonZeroWinding (false);
            ringClip.addPath (innerHole);
            g.reduceClipRegion (ringClip);
            g.setColour (juce::Colour (0x33000000));
            for (float k = 0.88f; k < 1.0f; k += 0.025f)
                g.drawEllipse (cx - bevelOuterR * k, cy - bevelOuterR * k,
                                bevelOuterR * k * 2.0f, bevelOuterR * k * 2.0f, 0.3f);
            g.setColour (juce::Colour (0x22FFFFFF));
            for (float k = 0.895f; k < 1.0f; k += 0.025f)
                g.drawEllipse (cx - bevelOuterR * k + 0.3f, cy - bevelOuterR * k + 0.3f,
                                bevelOuterR * k * 2.0f, bevelOuterR * k * 2.0f, 0.25f);
        }
        // Outer rim outline + top-edge highlight
        g.setColour (juce::Colour (0xFF000000));
        g.drawEllipse (cx - bevelOuterR, cy - bevelOuterR,
                        bevelOuterR * 2.0f, bevelOuterR * 2.0f, 0.7f);
        g.setColour (juce::Colour (0xDDFFFFFF));
        g.drawEllipse (cx - bevelOuterR + 0.5f, cy - bevelOuterR + 0.5f,
                        (bevelOuterR - 0.5f) * 2.0f, (bevelOuterR - 0.5f) * 2.0f, 0.45f);

        // ----- (4) Dark groove between bevel and body -----
        g.setColour (juce::Colour (0xFF000000));
        g.fillEllipse (cx - bevelInnerR - 0.5f, cy - bevelInnerR - 0.5f,
                        (bevelInnerR + 0.5f) * 2.0f, (bevelInnerR + 0.5f) * 2.0f);

        // ----- (5) Glossy bakelite body — radial gradient (curvature feel) -----
        {
            juce::ColourGradient bodyGr (juce::Colour (0xFF2A2A2A),
                                           cx - bodyR * 0.22f, cy - bodyR * 0.42f,
                                           juce::Colour (0xFF010101),
                                           cx + bodyR * 0.25f, cy + bodyR * 0.55f, true);
            bodyGr.addColour (0.30, juce::Colour (0xFF161616));
            bodyGr.addColour (0.60, juce::Colour (0xFF080808));
            bodyGr.addColour (0.88, juce::Colour (0xFF020202));
            g.setGradientFill (bodyGr);
            g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        }

        // ----- (6) Asymmetric upper-left glossy highlight — warm bakelite gloss -----
        {
            juce::ColourGradient hiGr (juce::Colour (0x66E8DFC8),
                                         cx - bodyR * 0.35f, cy - bodyR * 0.45f,
                                         juce::Colour (0x00E8DFC8),
                                         cx + bodyR * 0.30f, cy + bodyR * 0.20f, true);
            g.setGradientFill (hiGr);
            g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        }
        // Secondary tighter highlight
        {
            juce::ColourGradient hi2Gr (juce::Colour (0xAAF5EFD8),
                                          cx - bodyR * 0.30f, cy - bodyR * 0.55f,
                                          juce::Colour (0x00F5EFD8),
                                          cx + bodyR * 0.05f, cy - bodyR * 0.15f, true);
            g.setGradientFill (hi2Gr);
            g.fillEllipse (cx - bodyR * 0.70f, cy - bodyR * 0.85f,
                            bodyR * 0.95f, bodyR * 0.65f);
        }

        // ----- (7) INSET CENTER DISK — recessed (CACHED outer shadow) -----
        {
            juce::Path insetShadowPath;
            insetShadowPath.addEllipse (cx - insetR, cy - insetR,
                                          insetR * 2.0f, insetR * 2.0f);
            knobInsetShadow.render (g, insetShadowPath);
        }
        {
            juce::ColourGradient insGr (juce::Colour (0xFF000000),
                                          cx - insetR * 0.20f, cy - insetR * 0.35f,
                                          juce::Colour (0xFF222220),
                                          cx + insetR * 0.40f, cy + insetR * 0.60f, true);
            insGr.addColour (0.55, juce::Colour (0xFF070706));
            g.setGradientFill (insGr);
            g.fillEllipse (cx - insetR, cy - insetR, insetR * 2.0f, insetR * 2.0f);
        }
        // Top-edge inner cast shadow
        {
            juce::ColourGradient topShadow (juce::Colour (0xEE000000),
                                              cx, cy - insetR,
                                              juce::Colour (0x00000000),
                                              cx, cy - insetR * 0.05f, false);
            g.setGradientFill (topShadow);
            g.fillEllipse (cx - insetR, cy - insetR, insetR * 2.0f, insetR * 2.0f);
        }
        // Bottom warm bounce-light
        {
            juce::ColourGradient botGlow (juce::Colour (0x00000000),
                                            cx, cy + insetR * 0.30f,
                                            juce::Colour (0x33C8B888),
                                            cx, cy + insetR * 0.95f, false);
            g.setGradientFill (botGlow);
            g.fillEllipse (cx - insetR, cy - insetR, insetR * 2.0f, insetR * 2.0f);
        }
        g.setColour (juce::Colour (0xFF000000));
        g.drawEllipse (cx - insetR, cy - insetR, insetR * 2.0f, insetR * 2.0f, 0.55f);
        g.setColour (juce::Colour (0x33FFFFFF));
        g.drawEllipse (cx - insetR + 0.5f, cy - insetR + 0.5f,
                        (insetR - 0.5f) * 2.0f, (insetR - 0.5f) * 2.0f, 0.35f);

        // ----- (8) INDICATOR DOT — bright cream-white dot rotating with value -----
        // 5-layer: outer glow halo · drop shadow · body gradient · hot-spot · rim.
        // The outer glow halo makes the indicator clearly visible against the
        // dark bakelite body even when the user glances quickly.
        {
            const float dx = cx + sinA * dotOrbit;
            const float dy = cy - cosA * dotOrbit;
            // (8a) Soft cream-tinted glow halo (subtle "phosphor" feel)
            {
                juce::ColourGradient haloGr (juce::Colour (0x55F5EFD8),
                                                dx, dy,
                                                juce::Colour (0x00F5EFD8),
                                                dx + dotR * 2.0f, dy, true);
                g.setGradientFill (haloGr);
                g.fillEllipse (dx - dotR * 2.0f, dy - dotR * 2.0f,
                                dotR * 4.0f, dotR * 4.0f);
            }
            // (8b) Drop shadow
            g.setColour (juce::Colour (0xAA000000));
            g.fillEllipse (dx - dotR + 0.4f, dy - dotR + 0.55f,
                            dotR * 2.0f, dotR * 2.0f);
            // (8c) Cream-white body — radial gradient (bright top, warm rim)
            {
                juce::ColourGradient dGr (juce::Colour (0xFFFFFFFF),
                                            dx - dotR * 0.30f, dy - dotR * 0.40f,
                                            juce::Colour (0xFFC0BAA0),
                                            dx + dotR * 0.40f, dy + dotR * 0.50f, true);
                dGr.addColour (0.50, juce::Colour (0xFFF0E8C8));
                g.setGradientFill (dGr);
                g.fillEllipse (dx - dotR, dy - dotR, dotR * 2.0f, dotR * 2.0f);
            }
            // (8d) Sharp specular hot-spot
            g.setColour (juce::Colour (0xEEFFFFFF));
            g.fillEllipse (dx - dotR * 0.40f, dy - dotR * 0.55f,
                            dotR * 0.65f, dotR * 0.32f);
            // (8e) Dot rim outline
            g.setColour (juce::Colour (0xFF0A0A0A));
            g.drawEllipse (dx - dotR, dy - dotR, dotR * 2.0f, dotR * 2.0f, 0.35f);
        }

        juce::ignoreUnused (s);
    }

    void WireframeLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                                      juce::Button& b,
                                                      const juce::Colour& /*bg*/,
                                                      bool isOver, bool isDown)
    {
        const auto bounds = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool isOn = b.getToggleState();

        // ===== Cached melatonin drop shadow under raised button (UAD-grade) =====
        if (! isDown)
        {
            juce::Path shadowPath;
            shadowPath.addRoundedRectangle (bounds, 1.8f);
            buttonDropShadow.render (g, shadowPath);
        }

        // ===== Plastic face — UAD-style: subtle red WASH when on (no obnoxious mid-button LED) =====
        // Off-state: clean off-white plastic.
        // On-state: warm red-tinted face (like UAD glow buttons) — still legible.
        const auto faceHi  = isOn ? juce::Colour (0xFFE8B8A8) : juce::Colour (0xFFFAF7ED);
        const auto faceMd1 = isOn ? juce::Colour (0xFFD8A090) : juce::Colour (0xFFEBE8DC);
        const auto faceMd2 = isOn ? juce::Colour (0xFFC08878) : juce::Colour (0xFFD4D1C4);
        const auto faceLo  = isOn ? juce::Colour (0xFF986850) : juce::Colour (0xFFA8A498);

        juce::ColourGradient grad (faceHi, bounds.getX(), bounds.getY(),
                                    faceLo, bounds.getX(), bounds.getBottom(), false);
        grad.addColour (0.30, faceMd1);
        grad.addColour (0.55, faceMd2);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, 1.8f);

        // ===== Glossy top reflection (gradient overlay for plastic sheen) =====
        if (! isDown)
        {
            juce::ColourGradient glossGrad (juce::Colour (0x66FFFFFF), bounds.getX(), bounds.getY(),
                                              juce::Colour (0x00FFFFFF), bounds.getX(),
                                              bounds.getY() + bounds.getHeight() * 0.55f, false);
            g.setGradientFill (glossGrad);
            g.fillRoundedRectangle (bounds.withHeight (bounds.getHeight() * 0.55f), 1.8f);
        }

        // ===== Inset shadow when pressed (top-left dark band) =====
        if (isDown)
        {
            g.setColour (juce::Colour (0x55000000));
            g.fillRoundedRectangle (bounds.withHeight (bounds.getHeight() * 0.3f), 1.8f);
        }

        // Bright top edge (bevel highlight)
        g.setColour (juce::Colour (0xDDFFFFFF));
        g.drawLine (bounds.getX() + 1.8f, bounds.getY() + 0.7f,
                     bounds.getRight() - 1.8f, bounds.getY() + 0.7f, 0.8f);
        // Subtle second highlight 1px below
        g.setColour (juce::Colour (0x66FFFFFF));
        g.drawLine (bounds.getX() + 1.8f, bounds.getY() + 1.7f,
                     bounds.getRight() - 1.8f, bounds.getY() + 1.7f, 0.4f);

        // Dark bottom edge (bevel shadow)
        g.setColour (juce::Colour (0xAA000000));
        g.drawLine (bounds.getX() + 1.8f, bounds.getBottom() - 0.7f,
                     bounds.getRight() - 1.8f, bounds.getBottom() - 0.7f, 0.8f);

        // Left/right subtle edges
        g.setColour (juce::Colour (0x66FFFFFF));
        g.drawLine (bounds.getX() + 0.5f, bounds.getY() + 1.8f,
                     bounds.getX() + 0.5f, bounds.getBottom() - 1.8f, 0.5f);
        g.setColour (juce::Colour (0x55000000));
        g.drawLine (bounds.getRight() - 0.5f, bounds.getY() + 1.8f,
                     bounds.getRight() - 0.5f, bounds.getBottom() - 1.8f, 0.5f);

        // Outline (slightly thicker on hover for affordance)
        g.setColour (juce::Colour (kStroke));
        g.drawRoundedRectangle (bounds, 1.8f, isOver ? 1.1f : 0.7f);

        // ===== Tiny LED indicator — TOP-RIGHT CORNER (UAD-style pinprick) =====
        // Small + tucked in corner so it never interferes with button content.
        if (isOn)
        {
            const auto lx = bounds.getRight() - 3.5f;
            const auto ly = bounds.getY() + 3.0f;
            // Soft outer glow (small)
            g.setColour (juce::Colour (0x55FF3020));
            g.fillEllipse (lx - 2.5f, ly - 2.5f, 5.0f, 5.0f);
            // LED body
            g.setColour (juce::Colour (kRedLed));
            g.fillEllipse (lx - 1.4f, ly - 1.4f, 2.8f, 2.8f);
            // Tiny specular pinpoint
            g.setColour (juce::Colour (0xEEFFFFFF));
            g.fillEllipse (lx - 0.9f, ly - 1.0f, 0.9f, 0.6f);
        }
    }

    //==========================================================================
    //  drawButtonText — auto-switches to fontaudio typeface when the label is
    //  a PUA codepoint (0xF100–0xF1FF). Falls through to standard rendering
    //  for plain text buttons.
    //==========================================================================
    void WireframeLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b,
                                                 bool isOver, bool isDown)
    {
        const auto text = b.getButtonText();
        const bool isFontAudioGlyph = text.isNotEmpty()
            && (juce::juce_wchar) text[0] >= 0xf100
            && (juce::juce_wchar) text[0] <= 0xf1ff;

        const auto bounds  = b.getLocalBounds().reduced (2);
        const auto baseCol = juce::Colour (0xFF2A2620);

        if (isFontAudioGlyph)
        {
            const float h = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.72f;
            icons::drawIcon (g, text, bounds, baseCol.withAlpha (isDown ? 0.85f : 1.0f), h);
        }
        else
        {
            // Standard text path — match JUCE default behaviour
            const auto font = b.getLookAndFeel().getTextButtonFont (b, bounds.getHeight());
            g.setFont   (font);
            g.setColour (baseCol);
            g.drawFittedText (text, bounds, juce::Justification::centred, 1);
        }
        juce::ignoreUnused (isOver);
    }

    void WireframeLookAndFeel::drawToggleButton (juce::Graphics& g,
                                                   juce::ToggleButton& b,
                                                   bool isOver, bool isDown)
    {
        // UAD-style: button text is silkscreen LABEL below (editor paint draws it).
        // The button face itself stays clean — no overlapping text/LED conflict.
        drawButtonBackground (g, b, juce::Colours::white, isOver, isDown);
    }

    void WireframeLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h,
                                              bool /*down*/, int, int, int, int,
                                              juce::ComboBox& box)
    {
        const auto bounds = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);

        // UAD-grade recessed black panel (8 layers)
        // (1) Outer dark frame
        g.setColour (juce::Colour (0xFF050505));
        g.fillRoundedRectangle (bounds, 1.8f);
        // (2) Inner well — 3-stop vertical gradient with subtle warm bottom
        juce::ColourGradient grad (juce::Colour (0xFF101010), bounds.getX(), bounds.getY(),
                                    juce::Colour (0xFF2A2A2A), bounds.getX(), bounds.getBottom(), false);
        grad.addColour (0.5, juce::Colour (0xFF1A1A1A));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds.reduced (0.6f), 1.3f);
        // (3) Top inner shadow (recessed inset feel)
        juce::ColourGradient innerShadow (juce::Colour (0xCC000000), bounds.getX(), bounds.getY() + 0.6f,
                                            juce::Colour (0x00000000), bounds.getX(),
                                            bounds.getY() + 4.0f, false);
        g.setGradientFill (innerShadow);
        g.fillRoundedRectangle (bounds.reduced (0.6f).withHeight (3.5f), 1.0f);
        // (4) Top bright edge (chrome rim)
        g.setColour (juce::Colour (0x66FFFFFF));
        g.drawLine (bounds.getX() + 2.0f, bounds.getY() + 0.5f,
                     bounds.getRight() - 2.0f, bounds.getY() + 0.5f, 0.4f);
        // (5) Bottom bright edge
        g.setColour (juce::Colour (0x33FFFFFF));
        g.drawLine (bounds.getX() + 2.0f, bounds.getBottom() - 0.5f,
                     bounds.getRight() - 2.0f, bounds.getBottom() - 0.5f, 0.3f);
        // (6) Outline
        g.setColour (juce::Colour (kSilkDim));
        g.drawRoundedRectangle (bounds, 1.8f, 0.6f);

        // (7) Dropdown caret — crisp fontaudio glyph (retina-clean, replaces path)
        const auto caretR = juce::Rectangle<float> (bounds.getRight() - 14.0f,
                                                       bounds.getCentreY() - 5.0f,
                                                       12.0f, 10.0f);
        // Drop shadow
        icons::drawIcon (g, icons::CaretDown,
                          caretR.translated (0.4f, 0.6f),
                          juce::Colour (0xAA000000), 8.0f);
        // Bright chrome body
        icons::drawIcon (g, icons::CaretDown, caretR,
                          juce::Colour (0xFFE8E8E8), 8.0f);

        juce::ignoreUnused (box);
    }

    //==========================================================================
    //  Premium UAD-style tooltip
    //==========================================================================
    juce::Rectangle<int> WireframeLookAndFeel::getTooltipBounds (const juce::String& tipText,
                                                                   juce::Point<int> screenPos,
                                                                   juce::Rectangle<int> parentArea)
    {
        const juce::TextLayout tl;
        // Compute approximate size from text length (line by line)
        int maxLineLen = 0;
        int lineCount  = 0;
        juce::StringArray lines;
        lines.addLines (tipText);
        for (const auto& line : lines)
        {
            maxLineLen = juce::jmax (maxLineLen, line.length());
            ++lineCount;
        }
        const int w = juce::jmax (160, maxLineLen * 6 + 24);
        const int h = lineCount * 14 + 16;
        const int x = juce::jlimit (parentArea.getX(),
                                       parentArea.getRight()  - w,
                                       screenPos.x + 14);
        const int y = juce::jlimit (parentArea.getY(),
                                       parentArea.getBottom() - h,
                                       screenPos.y + 18);
        return { x, y, w, h };
    }

    void WireframeLookAndFeel::drawTooltip (juce::Graphics& g,
                                              const juce::String& text,
                                              int width, int height)
    {
        const auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height);

        // (1) Multi-stack soft drop shadow (UAD-grade depth)
        for (int i = 4; i >= 1; --i)
        {
            g.setColour (juce::Colour::fromFloatRGBA (0, 0, 0, 0.18f / (float) i));
            g.fillRoundedRectangle (bounds.translated ((float) i * 0.4f, (float) i * 0.8f)
                                          .expanded ((float) i * 0.4f), 4.0f);
        }

        // (2) Outer dark frame
        g.setColour (juce::Colour (0xFF050505));
        g.fillRoundedRectangle (bounds, 4.0f);

        // (3) Inner panel — dark matte with vertical gradient
        juce::ColourGradient panelGr (juce::Colour (0xFF1E1E1E), 0, 0,
                                        juce::Colour (0xFF101010), 0, (float) height, false);
        panelGr.addColour (0.5, juce::Colour (0xFF1A1A1A));
        g.setGradientFill (panelGr);
        g.fillRoundedRectangle (bounds.reduced (1.0f), 3.0f);

        // (4) Top chrome edge
        g.setColour (juce::Colour (0x99FFFFFF));
        g.drawLine (3.0f, 1.5f, (float) width - 3, 1.5f, 0.6f);
        // (5) Bottom shadow edge
        g.setColour (juce::Colour (0x77000000));
        g.drawLine (3.0f, (float) height - 1.5f, (float) width - 3, (float) height - 1.5f, 0.6f);
        // (6) Chrome outline
        g.setColour (juce::Colour (0xFF3A3A3A));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 0.6f);

        // (7) Text — first line bold (title), rest body
        juce::StringArray lines;
        lines.addLines (text);
        const juce::Font titleFont = juce::Font (juce::FontOptions (10.0f, juce::Font::bold))
                                          .withExtraKerningFactor (0.10f);
        const juce::Font bodyFont  (juce::FontOptions (9.0f));
        int y = 6;
        for (int i = 0; i < lines.size(); ++i)
        {
            if (i == 0)
            {
                // Title with engraved emboss effect
                g.setFont (titleFont);
                g.setColour (juce::Colours::black.withAlpha (0.7f));
                g.drawText (lines[i], juce::Rectangle<int> (10, y + 1, width - 20, 14),
                             juce::Justification::centredLeft, false);
                g.setColour (juce::Colour (0xFFE8E5DC));
                g.drawText (lines[i], juce::Rectangle<int> (10, y, width - 20, 14),
                             juce::Justification::centredLeft, false);
            }
            else
            {
                g.setFont (bodyFont);
                g.setColour (juce::Colour (0xFFB8B5AC));
                g.drawText (lines[i], juce::Rectangle<int> (10, y, width - 20, 14),
                             juce::Justification::centredLeft, false);
            }
            y += 14;
        }
    }

    void WireframeLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
    {
        // Tight arrow padding (12 px instead of JUCE default 30) → text visually centered
        // for small combos. Justification centered both horizontally and vertically.
        label.setBounds (1, 1, box.getWidth() - 14, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setJustificationType (juce::Justification::centred);
    }

    juce::Font WireframeLookAndFeel::getComboBoxFont (juce::ComboBox&)
    {
        return juce::Font (juce::FontOptions (9.5f, juce::Font::bold));
    }

    juce::Font WireframeLookAndFeel::getLabelFont (juce::Label& l)
    {
        // ComboBox embeds a Label internally; ensure it inherits readable size
        if (auto* parent = l.getParentComponent())
            if (dynamic_cast<juce::ComboBox*> (parent) != nullptr)
                return juce::Font (juce::FontOptions (9.5f, juce::Font::bold));
        return juce::Font (juce::FontOptions (9.0f));
    }

    //==========================================================================
    //  WireframeReelDeck — two animated wire-reels
    //==========================================================================
    WireframeReelDeck::WireframeReelDeck()
    {
        setInterceptsMouseClicks (false, false);
    }

    void WireframeReelDeck::setSpeed (int idx)
    {
        speedFactor = idx == 0 ? 0.5f : idx == 1 ? 1.0f : 2.0f;
    }

    void WireframeReelDeck::onVBlank()
    {
        if (! active)
            return;

        // Constant linear tape velocity; reels spin in opposite directions
        const float baseRpm = 0.04f * speedFactor;
        angleL -= baseRpm;
        angleR -= baseRpm;
        repaint();
    }

    void WireframeReelDeck::paint (juce::Graphics& g)
    {
        TRACE_COMPONENT();
        const auto bounds = getLocalBounds().toFloat();
        const auto sx = bounds.getWidth() / 800.0f;
        const auto sy = bounds.getHeight() / 310.0f;
        const auto r  = 130.0f * juce::jmin (sx, sy);

        static const auto reelL = getReelLeft();
        static const auto reelR = getReelRight();

        auto drawReel = [&] (const juce::Image& img, float cx, float cy, float angle)
        {
            if (img.isNull())
                return;

            // Drop shadow under reel (real Gaussian)
            g.setColour (juce::Colour (0x66000000));
            g.fillEllipse (cx - r - 1, cy - r + 5, (r + 1) * 2, (r + 1) * 2);

            // Photo-real reel — rotated bitmap drawn centered at (cx,cy)
            const float reelSize = r * 2.0f;
            juce::AffineTransform xform
                = juce::AffineTransform::translation (-img.getWidth()  * 0.5f,
                                                       -img.getHeight() * 0.5f)
                .scaled (reelSize / (float) img.getWidth(),
                         reelSize / (float) img.getHeight())
                .rotated (angle)
                .translated (cx, cy);

            g.drawImageTransformed (img, xform, false);
        };

        const float leftCx  = bounds.getX() + 220.0f * sx;
        const float rightCx = bounds.getX() + 580.0f * sx;
        const float cy      = bounds.getY() + 147.0f * sy;
        drawReel (reelL, leftCx,  cy, angleL);
        drawReel (reelR, rightCx, cy, angleR);
    }

    //==========================================================================
    //  WireframeVU
    //==========================================================================
    WireframeVU::WireframeVU (const juce::String& chLabel) : label (chLabel)
    {
        setInterceptsMouseClicks (false, false);
    }

    void WireframeVU::setLevel (float dbfs)
    {
        // 300ms ballistics (300 ms = ~0.05 per 16ms tick * 30Hz)
        const float target = juce::jlimit (-60.0f, 6.0f, dbfs);
        current += (target - current) * 0.18f;
        // Peak-hold ballistics: instant rise, slow decay (0.6 dB/tick → ~1.7s back to current)
        if (target > peakHold)
            peakHold = target;
        else
            peakHold = juce::jmax (current, peakHold - 0.6f);
        repaint();
    }

    void WireframeVU::paint (juce::Graphics& g)
    {
        TRACE_COMPONENT();
        // Beocord 2000 De Luxe VU — UAD-grade 18-layer rendering
        // Per manual p.12: 0-8 black "Mischbereich + Normal" · 8-10 red "Übersteuerung"
        const auto bounds = getLocalBounds().toFloat();

        // ===== (1) Chrome molded bezel — 5-stop polished gradient =====
        juce::ColourGradient bezelGrad (juce::Colour (0xFFFAFAFA),
                                         bounds.getX(), bounds.getY(),
                                         juce::Colour (0xFF555555),
                                         bounds.getX(), bounds.getBottom(), false);
        bezelGrad.addColour (0.25, juce::Colour (0xFFD8D8D8));
        bezelGrad.addColour (0.50, juce::Colour (0xFFB0B0B0));
        bezelGrad.addColour (0.75, juce::Colour (0xFF858585));
        g.setGradientFill (bezelGrad);
        g.fillRoundedRectangle (bounds, 3.5f);

        // ===== (2) Brushed-chrome horizontal striations on bezel =====
        g.setColour (juce::Colour (0x10000000));
        for (int yy = 1; yy < (int) bounds.getHeight() - 1; yy += 2)
            g.drawLine (bounds.getX() + 2, bounds.getY() + yy,
                         bounds.getRight() - 2, bounds.getY() + yy, 0.25f);

        // ===== (3) Top edge highlight (cylinder-cap reflection) =====
        g.setColour (juce::Colour (0xEEFFFFFF));
        g.drawLine (bounds.getX() + 3, bounds.getY() + 0.8f,
                     bounds.getRight() - 3, bounds.getY() + 0.8f, 0.9f);
        g.setColour (juce::Colour (0x55FFFFFF));
        g.drawLine (bounds.getX() + 3, bounds.getY() + 1.8f,
                     bounds.getRight() - 3, bounds.getY() + 1.8f, 0.4f);
        // Bottom edge highlight (subtle)
        g.setColour (juce::Colour (0xAAFFFFFF));
        g.drawLine (bounds.getX() + 3, bounds.getBottom() - 0.8f,
                     bounds.getRight() - 3, bounds.getBottom() - 0.8f, 0.6f);

        // ===== (4) Corner bevel highlights (4 bright dots, molded-plastic feel) =====
        g.setColour (juce::Colour (0xDDFFFFFF));
        g.fillEllipse (bounds.getX() + 1.0f, bounds.getY() + 1.0f, 2.2f, 2.2f);
        g.fillEllipse (bounds.getRight() - 3.2f, bounds.getY() + 1.0f, 2.2f, 2.2f);
        g.setColour (juce::Colour (0x99FFFFFF));
        g.fillEllipse (bounds.getX() + 1.0f, bounds.getBottom() - 3.2f, 2.2f, 2.2f);
        g.fillEllipse (bounds.getRight() - 3.2f, bounds.getBottom() - 3.2f, 2.2f, 2.2f);

        // ===== (5) Outer bezel outline =====
        g.setColour (juce::Colour (0xFF0E0E0E));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 3.5f, 0.7f);

        // ===== (6) Recessed dark inner well — deep dark with vertical gradient =====
        const auto well = bounds.reduced (3.8f, 3.2f);
        juce::ColourGradient wellGrad (juce::Colour (0xFF050505),
                                         well.getX(), well.getY(),
                                         juce::Colour (0xFF1E1E1E),
                                         well.getX(), well.getBottom(), false);
        g.setGradientFill (wellGrad);
        g.fillRoundedRectangle (well, 1.5f);
        // Well rim shadow (recessed feel)
        g.setColour (juce::Colour (0xCC000000));
        g.drawRoundedRectangle (well, 1.5f, 0.8f);

        // ===== (7) Cream meter face inside the well (with subtle aging) =====
        const auto face = well.reduced (1.6f, 1.2f);
        const float boundary = face.getX() + face.getWidth() * 0.80f;

        // Black field (0–8) — gradient for slight depth
        juce::ColourGradient blackGrad (juce::Colour (0xFF0A0A0A),
                                          face.getX(), face.getY(),
                                          juce::Colour (0xFF202020),
                                          face.getX(), face.getBottom(), false);
        g.setGradientFill (blackGrad);
        g.fillRect (face.getX(), face.getY(), boundary - face.getX(), face.getHeight());

        // ===== (8) Red field (8–10) — UAD-grade gradient + subtle glow =====
        juce::ColourGradient redGrad (juce::Colour (0xFFFF5040),
                                        boundary, face.getY(),
                                        juce::Colour (0xFFB81818),
                                        boundary, face.getBottom(), false);
        redGrad.addColour (0.5, juce::Colour (0xFFE03020));
        g.setGradientFill (redGrad);
        g.fillRect (boundary, face.getY(), face.getRight() - boundary, face.getHeight());

        // Soft red glow halo (extends slightly into black field)
        juce::ColourGradient glowGrad (juce::Colour (0x55FF3020), boundary, face.getCentreY(),
                                         juce::Colour (0x00FF3020), boundary - 5.0f, face.getCentreY(),
                                         false);
        g.setGradientFill (glowGrad);
        g.fillRect (boundary - 5.0f, face.getY(), 5.0f, face.getHeight());

        // ===== (9) White vertical calibration line at boundary (8.0) =====
        g.setColour (juce::Colour (0xFFFFFFFF));
        g.drawLine (boundary, face.getY(), boundary, face.getBottom(), 0.8f);

        // ===== (10) White tick marks with varied lengths =====
        for (int i = 0; i <= 10; ++i)
        {
            const float tx = face.getX() + face.getWidth() * ((float) i / 10.0f);
            const bool isMajor = (i % 2 == 0);
            const float th = isMajor ? 3.0f : 1.5f;
            const bool inRed = (i > 8);
            g.setColour (inRed ? juce::Colour (0xFFFFE0E0)
                               : juce::Colour (0xEEFFFFFF));
            g.drawLine (tx, face.getY(), tx, face.getY() + th, isMajor ? 0.5f : 0.35f);
        }

        // ===== (11) Numerical labels — bold sans-serif per B&O typography =====
        g.setFont (juce::Font (juce::FontOptions (4.0f, juce::Font::bold)));
        const int marks[] = { 0, 4, 8, 10 };
        for (int m : marks)
        {
            const float tx = face.getX() + face.getWidth() * ((float) m / 10.0f);
            const bool inRed = (m > 8);
            g.setColour (inRed ? juce::Colour (0xFFFFE0E0) : juce::Colour (0xFFFFFFFF));
            g.drawText (juce::String (m),
                         juce::Rectangle<int> ((int) tx - 4, (int) face.getY() + 3, 8, 4),
                         juce::Justification::centred, false);
        }

        // ===== (12) "VU" brand mark — bottom-right, authentic typography =====
        g.setColour (juce::Colour (0x77FFFFFF));
        g.setFont (juce::Font (juce::FontOptions (3.5f, juce::Font::bold))
                       .withExtraKerningFactor (0.20f));
        g.drawText ("VU",
                     juce::Rectangle<int> ((int) face.getRight() - 10, (int) face.getBottom() - 5, 9, 4),
                     juce::Justification::centredRight, false);

        // ===== (13a) Peak-hold marker — thin amber tick above face =====
        // Authentic vintage broadcast meter behaviour: instant rise, slow decay.
        const float peakNorm = juce::jmap (juce::jlimit (-40.0f, 0.0f, peakHold),
                                              -40.0f, 0.0f, 0.0f, 1.0f);
        const float peakX = face.getX() + face.getWidth() * juce::jlimit (0.0f, 1.0f, peakNorm);
        const bool peakInRed = peakNorm > 0.80f;
        // Render only if peak is meaningfully above current needle (avoid clutter at idle)
        if (peakHold > current + 0.5f && peakHold > -39.0f)
        {
            // Soft glow halo
            g.setColour ((peakInRed ? juce::Colour (0xFFFF6040) : juce::Colour (0xFFFFD080))
                            .withAlpha (0.35f));
            g.drawLine (peakX, face.getY() + 1.0f, peakX, face.getBottom() - 1.0f, 1.8f);
            // Crisp body
            g.setColour (peakInRed ? juce::Colour (0xFFFF8060) : juce::Colour (0xFFFFE8A0));
            g.drawLine (peakX, face.getY() + 1.0f, peakX, face.getBottom() - 1.0f, 0.7f);
        }

        // ===== (13) Needle — tapered with shadow and brighter tip =====
        const float norm  = juce::jmap (juce::jlimit (-40.0f, 0.0f, current),
                                          -40.0f, 0.0f, 0.0f, 1.0f);
        const float needleX = face.getX() + face.getWidth() * juce::jlimit (0.0f, 1.0f, norm);
        const bool inRedZone = norm > 0.80f;

        // Needle drop shadow (offset right+down)
        g.setColour (juce::Colour (0x99000000));
        g.drawLine (needleX + 0.7f, face.getY() + 0.6f,
                     needleX + 0.7f, face.getBottom() + 0.4f, 0.9f);
        // Needle body — white over black, red-tinted over red zone
        g.setColour (inRedZone ? juce::Colour (0xFFFFE8E0) : juce::Colour (0xFFFFFFFF));
        g.drawLine (needleX, face.getY(), needleX, face.getBottom(), 1.0f);
        // Needle inner bright core (gives 3D feel)
        g.setColour (juce::Colour (0xCCFFFFFF));
        g.drawLine (needleX - 0.2f, face.getY() + 1.0f,
                     needleX - 0.2f, face.getBottom() - 0.5f, 0.4f);

        // ===== (14) Pivot screw at bottom center =====
        const float pivotX = face.getCentreX();
        const float pivotY = face.getBottom() - 1.5f;
        g.setColour (juce::Colour (0xFF080808));
        g.fillEllipse (pivotX - 1.8f, pivotY - 1.8f, 3.6f, 3.6f);
        juce::ColourGradient pivotGrad (juce::Colour (0xFFEEEEEE), pivotX - 0.5f, pivotY - 1.0f,
                                          juce::Colour (0xFF6E6E6E), pivotX + 1.0f, pivotY + 1.0f, false);
        g.setGradientFill (pivotGrad);
        g.fillEllipse (pivotX - 1.3f, pivotY - 1.3f, 2.6f, 2.6f);
        // Phillips slot
        g.setColour (juce::Colour (0xAA000000));
        g.drawLine (pivotX - 0.9f, pivotY, pivotX + 0.9f, pivotY, 0.3f);

        // ===== (15) Curved glass dome reflection — Fresnel-style with arc =====
        // Multi-layer for realistic convex glass cover (like real VU meter)
        // Layer A: broad soft top reflection
        juce::ColourGradient glassA (juce::Colour (0x44FFFFFF), face.getX(), face.getY(),
                                       juce::Colour (0x00FFFFFF), face.getX(),
                                       face.getY() + face.getHeight() * 0.65f, false);
        g.setGradientFill (glassA);
        g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.55f), 1.0f);
        // Layer B: tighter arc highlight (curved glass edge catch)
        juce::Path domeArc;
        domeArc.addCentredArc (face.getCentreX(), face.getY() - face.getHeight() * 0.3f,
                                  face.getWidth() * 0.55f, face.getHeight() * 0.50f, 0.0f,
                                  -juce::MathConstants<float>::halfPi * 0.7f,
                                   juce::MathConstants<float>::halfPi * 0.7f, true);
        domeArc.lineTo (face.getCentreX() + face.getWidth() * 0.35f,
                        face.getY() + face.getHeight() * 0.10f);
        domeArc.lineTo (face.getCentreX() - face.getWidth() * 0.35f,
                        face.getY() + face.getHeight() * 0.10f);
        domeArc.closeSubPath();
        g.setColour (juce::Colour (0x33FFFFFF));
        g.fillPath (domeArc);
        // Layer C: thin bright top-edge band (glass meniscus)
        g.setColour (juce::Colour (0x88FFFFFF));
        g.fillRect (face.getX() + 1.5f, face.getY() + 0.5f,
                     face.getWidth() - 3.0f, 0.7f);

        // ===== (16) Glass edge highlights (left + right of face) =====
        g.setColour (juce::Colour (0x44FFFFFF));
        g.drawLine (face.getX() + 0.3f, face.getY() + 1.0f,
                     face.getX() + 0.3f, face.getBottom() - 1.0f, 0.3f);
        g.setColour (juce::Colour (0x33000000));
        g.drawLine (face.getRight() - 0.3f, face.getY() + 1.0f,
                     face.getRight() - 0.3f, face.getBottom() - 1.0f, 0.3f);

        // ===== (17) Face outline =====
        g.setColour (juce::Colour (0xFF0A0A0A));
        g.drawRect (face.reduced (0.3f), 0.4f);
    }

    //==========================================================================
    //  WireframeEditor
    //==========================================================================
    WireframeEditor::WireframeEditor (BC2000DLProcessor& p)
        : juce::AudioProcessorEditor (p), audioProc (p)
    {
        setLookAndFeel (&lnf);
        setSize (kW, kH);
        setResizable (false, false);

        // -------- Fader setup helper --------
        auto setupFader = [&] (juce::Slider& s, const juce::String& paramId, double dblClickDef)
        {
            s.setSliderStyle (juce::Slider::LinearVertical);
            s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            s.setDoubleClickReturnValue (true, dblClickDef);
            s.setSliderSnapsToMousePosition (true);
            s.setVelocityBasedMode (false);
            s.setMouseDragSensitivity (160);
            s.setPopupDisplayEnabled (true, true, this, 1500);
            s.setMouseCursor (juce::MouseCursor::PointingHandCursor);
            addAndMakeVisible (s);
            if (paramId.isNotEmpty())
                sAtts.push_back (std::make_unique<SAtt> (audioProc.apvts, paramId, s));
        };

        // -------- Rotary knob helper --------
        auto setupKnob = [&] (juce::Slider& s, const juce::String& paramId, double dblClickDef)
        {
            s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            s.setDoubleClickReturnValue (true, dblClickDef);
            s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                    juce::MathConstants<float>::pi * 2.8f, true);
            s.setPopupDisplayEnabled (true, true, this, 1500);
            s.setMouseCursor (juce::MouseCursor::PointingHandCursor);
            addAndMakeVisible (s);
            if (paramId.isNotEmpty())
                sAtts.push_back (std::make_unique<SAtt> (audioProc.apvts, paramId, s));
        };

        // -------- Toggle button helper --------
        auto setupToggle = [&] (juce::ToggleButton& b, const juce::String& label,
                                 const juce::String& paramId)
        {
            b.setButtonText (label);
            b.setClickingTogglesState (true);
            b.setMouseCursor (juce::MouseCursor::PointingHandCursor);
            addAndMakeVisible (b);
            if (paramId.isNotEmpty())
                bAtts.push_back (std::make_unique<BAtt> (audioProc.apvts, paramId, b));
        };

        // -------- ComboBox helper --------
        auto setupCombo = [&] (juce::ComboBox& cb, const juce::StringArray& items,
                                const juce::String& paramId)
        {
            cb.addItemList (items, 1);
            cb.setMouseCursor (juce::MouseCursor::PointingHandCursor);
            addAndMakeVisible (cb);
            if (paramId.isNotEmpty())
            {
                cAtts.push_back (std::make_unique<CAtt> (audioProc.apvts, paramId, cb));
                // Explicit initial selection sync — ensures combo displays current value
                // immediately (JUCE 8 attachment sometimes lags by one event loop)
                if (auto* raw = audioProc.apvts.getRawParameterValue (paramId))
                {
                    const int idx = juce::jlimit (0, items.size() - 1, (int) raw->load());
                    cb.setSelectedItemIndex (idx, juce::dontSendNotification);
                }
            }
        };

        // ===== 10 faders (5 källor × L+R) =====
        setupFader (faderRadioL,  "radio_gain",     0.0);
        setupFader (faderRadioR,  "radio_gain_r",   0.0);
        setupFader (faderPhonoL,  "phono_gain",     0.0);
        setupFader (faderPhonoR,  "phono_gain_r",   0.0);
        setupFader (faderMicL,    "mic_gain",       0.5);
        setupFader (faderMicR,    "mic_gain_r",     0.5);
        setupFader (faderEchoL,   "echo_amount",    0.0);
        setupFader (faderEchoR,   "echo_amount_r",  0.0);
        // Master L+R båda till master_volume (synkroniseras via APVTS-broadcast)
        setupFader (faderMasterL, "master_volume",   0.85);
        setupFader (faderMasterR, "master_volume_r", 0.85);

        // Fader tooltips — descriptive hover info (popup display still shows live value)
        faderRadioL .setTooltip ("RADIO L (#13)\nLeft-channel gain for radio/tuner input.\nFeeds the 8904003 flat preamp (UW0029 + 2N2613).\nLog skew 0.55 — gentle 1968 curve.");
        faderRadioR .setTooltip ("RADIO R (#13)\nRight-channel gain for radio/tuner input.");
        faderPhonoL .setTooltip ("PHONO L (#14 ◇)\nLeft-channel gain for phono input.\nFeeds 8904002 with RIAA EQ (H-mode)\nor flat ceramic preamp (L-mode).");
        faderPhonoR .setTooltip ("PHONO R (#14 ◇)\nRight-channel gain for phono input.");
        faderMicL   .setTooltip ("MIC L (#15 ◇)\nLeft-channel gain for microphone input.\n50Ω/200Ω LoZ via 8012003 transformer,\nor HiZ direct (per MIC INPUT MODE).");
        faderMicR   .setTooltip ("MIC R (#15 ◇)\nRight-channel gain for microphone input.");
        faderEchoL  .setTooltip ("ECHO L (#10a)\nEcho SEND-niva (record->play-head-loop).\nKraver ECHO ON. Med S on S aktiv blir\nekot ping-pong (L<->R). Styr INTE\nsjalva S-on-S-lagernivan (den ar fast).");
        faderEchoR  .setTooltip ("ECHO R (#10a)\nEcho SEND-niva, hoger kanal.\nKraver ECHO ON.");
        faderMasterL.setTooltip ("MASTER L (#12)\nOutput level — vänster kanal.\nOberoende av MASTER R (egen skydepotentiometer).\nBALANCE-knoben viktar därutöver L/R.");
        faderMasterR.setTooltip ("MASTER R (#12)\nOutput level — höger kanal.\nOberoende av MASTER L.");

        // ===== 9 service-knobs + 2 selectors (authentic-restoration) =====
        setupKnob (knobBiasL,       "bias_amount",      1.0);
        setupKnob (knobBiasR,       "bias_amount_r",    1.0);
        setupKnob (knobSatL,        "saturation_drive", 1.0);
        setupKnob (knobSatR,        "saturation_drive_r", 1.0);
        setupKnob (knobWow,         "wow_flutter",      0.3);
        setupKnob (knobPrintTh,     "print_through",    0.0);
        setupKnob (knobStereoAsym,  "stereo_asymmetry", 0.02);
        setupKnob (knobMultiplay,   "multiplay_gen",    1.0);
        setupKnob (knobMainsHum,    "mains_hum",        0.0);
        setupCombo (cbTapeFormula,  { "AGFA", "BASF", "SCOTCH" }, "tape_formula");
        setupCombo (cbMainsHumFreq, { "50", "60" }, "mains_hum_freq");

        // Premium tooltips — engineer-grade descriptions
        knobBiasL      .setTooltip ("BIAS L\nHead-bias current for left channel.\nUnder-bias = brighter + grittier.\nOver-bias = darker + smoother.");
        knobBiasR      .setTooltip ("BIAS R\nHead-bias current for right channel.\nAdjust for L/R tape-head match.");
        knobSatL       .setTooltip ("SATURATION L\nTape saturation drive for left channel.\nHigher = more harmonic generation\n+ compression at peaks.");
        knobSatR       .setTooltip ("SATURATION R\nTape saturation drive for right channel.");
        knobWow        .setTooltip ("WOW & FLUTTER\nMechanical pitch instability\nfrom capstan + reel motors.\n0.3 = authentic 1968 spec.");
        knobPrintTh    .setTooltip ("PRINT THROUGH\nGhost pre/post-echo from\nadjacent tape layers (-60 dB).\nClassic vintage tape artifact.");
        knobStereoAsym .setTooltip ("STEREO ASYMMETRY\nL/R Ge-transistor mismatch.\n0.02 = authentic 1968 tolerance,\ngives natural stereo width.");
        knobMultiplay  .setTooltip ("MULTIPLAY GENERATION\nNumber of S-on-S overdub passes.\nEach gen adds HF rolloff +\nnoise-floor accumulation.");
        knobMainsHum   .setTooltip ("MAINS HUM\n50/60 Hz fundamental + 3rd harmonic.\nAuthentic 1968 amp character.\n0.0 = clean (default).");
        cbTapeFormula  .setTooltip ("TAPE FORMULA\nAGFA = soft, vintage warmth\nBASF = neutral, modern\nSCOTCH = aggressive, punchy");
        cbMainsHumFreq .setTooltip ("MAINS HUM FREQUENCY\n50 Hz = European mains\n60 Hz = US/Japan mains");

        // ===== Höger zon =====
        setupCombo (cbSpeed, { "4.75", "9.5", "19" }, "speed");
        setupKnob  (knobBass,       "bass_db",     0.0);
        setupKnob  (knobTreble,     "treble_db",   0.0);
        setupKnob  (knobBalance,    "balance",     0.0);   // #10 — bipolär L/R
        setupKnob  (knobInputTrim,  "input_trim",  0.0);
        setupKnob  (knobOutputTrim, "output_trim", 0.0);
        setupKnob  (knobMix,        "mix",         1.0);   // Fas 2 — dry/wet
        setupKnob  (knobTapeNoise,  "tape_noise",  1.0);   // Fas 2 — bandbrus-skala

        cbSpeed        .setTooltip ("TAPE SPEED\n4.75 cm/s = warmest, narrowest BW\n9.5 cm/s = nominal\n19 cm/s = brightest, widest BW");
        knobBass       .setTooltip ("BASS (#9)\nLow-shelf tone control\nfrom B&O manual.\nRange: -12 to +12 dB");
        knobTreble     .setTooltip ("TREBLE (#8 DISKANT)\nHigh-shelf tone control\nfrom B&O manual.\nRange: -12 to +12 dB");
        knobBalance    .setTooltip ("BALANCE (#10)\nL/R output balance.\nCenter = lika kanaler.\nVrid mot L/R för att vikta utgangen.");
        knobInputTrim  .setTooltip ("INPUT TRIM\nPre-DSP gain staging.\nLower hot DAW signals before\nthey hit tape saturation.\nRange: -24 to +24 dB");
        knobOutputTrim .setTooltip ("OUTPUT TRIM\nPost-DSP makeup gain.\nCompensate for tape-sat\nlevel reduction.\nRange: -24 to +24 dB");
        knobMix        .setTooltip ("MIX (dry/wet)\nParallel blend of the dry input\nwith the full tape chain.\nLatency-compensated → phase-correct.\n100% = fully wet (default).");
        knobTapeNoise  .setTooltip ("TAPE NOISE\nTape-hiss level (oxide self-noise).\n1.0 = authentic spec level,\n0 = silent, 2.0 = worn tape.");

        // Echo plugin-extension — now backed by real APVTS params + DSP
        setupToggle (tEchoPluginOn, "ON",   "echo_enabled");
        setupKnob   (knobEchoTime,  "echo_time",     150.0);
        setupKnob   (knobEchoFb,    "echo_feedback", 0.5);

        tEchoPluginOn.setTooltip ("ECHO ENABLED (#18)\nToggle tape-echo loop on/off.\nLED lights when active.");
        knobEchoTime .setTooltip ("ECHO TIME\nTape-head-to-head delay time.\nAuto: 75/150/300 ms by speed.\nUser override: 30-350 ms");
        knobEchoFb   .setTooltip ("ECHO FEEDBACK\nLoop recirculation amount.\n>0.85 enters self-oscillation\n(authentic tape behavior).");

        // ===== Left zone — rockers =====
        setupToggle (btnTrack1,      "TRK 1",  "track_1");
        setupToggle (btnTrack2,      "TRK 2",  "track_2");
        setupToggle (btnRecArm1,     "ARM L",  "rec_arm_1");
        setupToggle (btnRecArm2,     "ARM R",  "rec_arm_2");
        setupToggle (btnSync,        "SYNC",   "synchroplay");
        setupToggle (btnMoment,      "PAUSE",  "pause");
        setupToggle (btnPA,          "P.A.",   "pa_enabled");
        setupToggle (btnForstarkare, "AMP",    "bypass_tape");
        setupToggle (btnSoSOn,       "ON",     "sos_enabled");
        setupToggle (btnSpkInt,      "I",      "speaker_int");
        setupToggle (btnSpkExt,      "II",     "speaker_ext");
        setupToggle (btnSpkMute,     "MUTE",   "speaker_mute");

        // ===== Rocker-button tooltips (engineer-grade) =====
        btnTrack1     .setTooltip ("TRACK 1 (#19)\nMonitor track 1 (left).\nBoth on = stereo · only 1 = L-only.\nManual #19 monitor-routing logic.");
        btnTrack2     .setTooltip ("TRACK 2 (#20)\nMonitor track 2 (right).\nBoth on = stereo · only 2 = R-only.");
        btnRecArm1    .setTooltip ("REC ARM L (#24)\nArm left channel for recording.\nLED lights red when armed.\nRequired before tape-write enabled.");
        btnRecArm2    .setTooltip ("REC ARM R (#25)\nArm right channel for recording.");
        btnSync       .setTooltip ("SYNCHROPLAY (#26)\nUse record-head for playback monitoring.\nDrier sound — bypasses playback-EQ.\nUsed for overdub sync without latency.");
        btnMoment     .setTooltip ("PAUSE (#11 MOMENTANSTOP)\nMomentary stop — reels freeze in place.\nMutes output (master_volume → 0).\nClick again to resume.");
        btnPA         .setTooltip ("PUBLIC ADDRESS (manual s.8)\nDucks phono/radio when mic is active.\nClassic vintage live-PA function.");
        btnForstarkare.setTooltip ("BYPASS TAPE (#23 'FÖRSTÄRKARE')\nRoute mixer direct to output, skip\ntape pipeline. Use as line mixer\nwithout tape coloring.");
        btnSoSOn      .setTooltip ("SOUND ON SOUND (S on S)\nLagrar L<->R (mono-vard bounce, fast niva).\nKombinera med ECHO ON -> ping-pong-eko.\nOBS: SOS + ECHO + hog Multiplay staplar\nkorskoppling + mattnad = avsiktligt grungigt\n(autentiskt tape-S-on-S/dub-beteende).");
        btnSpkInt     .setTooltip ("SPEAKER I (#5 INT)\nInternal monitor speaker on.\nMutually exclusive with EXT / MUTE.");
        btnSpkExt     .setTooltip ("SPEAKER II (#4 EXT)\nExternal speaker output on.\nFeeds the power-amp stage (8004014).");
        btnSpkMute    .setTooltip ("SPEAKER MUTE (#6)\nKill all speaker output.\nHeadphones still active.");
        setupCombo  (cbMonitor, { "SOURCE", "TAPE" }, "monitor_mode");

        // ===== Per-source mode tabs (3 sources × mode selector) =====
        setupCombo  (cbRadioMode, { "L", "H" }, "radio_mode");
        setupCombo  (cbPhonoMode, { "L", "H" }, "phono_mode");
        setupCombo  (cbMicMode,   { "50Ω", "200Ω", "HiZ" }, "mic_mode");

        cbRadioMode  .setTooltip ("RADIO INPUT MODE\nL = 3 mV @ 47 kΩ (low)\nH = 100 mV @ 100 kΩ (high)");
        cbPhonoMode  .setTooltip ("PHONO INPUT MODE\nL = 40 mV @ 4 MΩ (ceramic)\nH = 2 mV @ 47 kΩ (magnetic)");
        cbMonitor    .setTooltip ("MONITOR (#22)\nSOURCE = pre-tape signal\nTAPE = post-tape playback\nFlip during record to verify.");
        cbMicMode    .setTooltip ("MIC INPUT MODE\n50Ω = LoZ dynamic (+6 dB pad)\n200Ω = LoZ studio mic\nHiZ = Crystal/high-impedance mic");

        // ===== Sub-components =====
        addAndMakeVisible (reelDeck);
        addAndMakeVisible (vuInL);
        addAndMakeVisible (vuInR);
        addAndMakeVisible (vuOutL);
        addAndMakeVisible (vuOutR);

        // ===== Preset bank UI =====
        btnPresetName.setButtonText (bc2000dl::kPresets[0].name);
        btnPresetName.setTooltip ("Click to open premium preset browser — search, categories, favorites, parameter preview");
        btnPresetName.onClick = [this]
        {
            browserBackdrop.setVisible (true);
            browserBackdrop.toFront    (false);
            presetBrowser.openBrowser (currentPresetIdx);
            presetBrowser.toFront     (true);
        };

        // Premium browser callback — fire preset and update name button
        presetBrowser.onPresetSelected = [this] (int idx)
        {
            currentPresetIdx = juce::jlimit (0, bc2000dl::kNumPresets - 1, idx);
            btnPresetName.setButtonText (bc2000dl::kPresets[currentPresetIdx].name);
            applyPreset (currentPresetIdx);
            browserBackdrop.setVisible (false);
        };
        // Browser hides itself on Esc / outside-click / close button —
        // sync the backdrop via ComponentListener::componentVisibilityChanged.
        presetBrowser.addComponentListener (this);
        browserBackdrop.onDismiss = [this]
        {
            presetBrowser.setVisible  (false);
            browserBackdrop.setVisible (false);
        };

        addChildComponent (browserBackdrop); // dim scrim behind browser
        addChildComponent (presetBrowser);   // hidden until openBrowser() is called

        // Premium prev/next icons (fontaudio) — handled by drawButtonText override
        btnPresetPrev.setButtonText (icons::Prev);
        btnPresetNext.setButtonText (icons::Next);
        btnPresetPrev.setTooltip ("Previous preset");
        btnPresetNext.setTooltip ("Next preset");
        btnPresetPrev.onClick = [this]
        {
            currentPresetIdx = (currentPresetIdx - 1 + bc2000dl::kNumPresets) % bc2000dl::kNumPresets;
            btnPresetName.setButtonText (bc2000dl::kPresets[currentPresetIdx].name);
            applyPreset (currentPresetIdx);
        };
        btnPresetNext.onClick = [this]
        {
            currentPresetIdx = (currentPresetIdx + 1) % bc2000dl::kNumPresets;
            btnPresetName.setButtonText (bc2000dl::kPresets[currentPresetIdx].name);
            applyPreset (currentPresetIdx);
        };
        addAndMakeVisible (btnPresetName);
        addAndMakeVisible (btnPresetPrev);
        addAndMakeVisible (btnPresetNext);

        // Högerklick på preset-namnet → user-preset-meny (spara/ladda/radera)
        btnPresetName.onRightClick = [this] { showUserPresetMenu(); };

        // ===== A/B-compare =====
        auto setupAb = [&] (juce::TextButton& b, const juce::String& tip)
        {
            b.setTooltip (tip);
            b.setMouseCursor (juce::MouseCursor::PointingHandCursor);
            addAndMakeVisible (b);
        };
        setupAb (btnAbA, "A/B: setting A.\nKlicka för att jamfora.\nHogerklick preset-namn for att spara.");
        setupAb (btnAbB, "A/B: setting B.\nKlicka for att jamfora med A.");
        setupAb (btnAbCopy, "Kopiera aktiv setting till den andra\n(matcha A och B som startpunkt).");
        btnAbA.onClick    = [this] { audioProc.abRecall (0); refreshAbButtons(); };
        btnAbB.onClick    = [this] { audioProc.abRecall (1); refreshAbButtons(); };
        btnAbCopy.onClick = [this] { audioProc.abCopyActiveToOther(); };
        refreshAbButtons();

        startTimerHz (30);
    }

    void WireframeEditor::refreshAbButtons()
    {
        const bool aActive = (audioProc.getABSlot() == 0);
        btnAbA.setToggleState (aActive,  juce::dontSendNotification);
        btnAbB.setToggleState (! aActive, juce::dontSendNotification);
    }

    void WireframeEditor::showUserPresetMenu()
    {
        juce::PopupMenu m;
        m.addItem (1, "Save User Preset...");
        m.addItem (2, "Reveal Presets Folder...");

        auto files = audioProc.listUserPresets();
        if (! files.isEmpty())
        {
            m.addSeparator();
            juce::PopupMenu loadSub, delSub;
            for (int i = 0; i < files.size(); ++i)
            {
                loadSub.addItem (1000 + i, files[i].getFileNameWithoutExtension());
                delSub .addItem (2000 + i, files[i].getFileNameWithoutExtension());
            }
            m.addSubMenu ("Load User Preset", loadSub);
            m.addSubMenu ("Delete User Preset", delSub);
        }

        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (btnPresetName),
            [this, files] (int r)
            {
                if (r == 1)
                {
                    auto aw = std::make_shared<juce::AlertWindow> (
                        "Save User Preset", "Namnge din preset:",
                        juce::MessageBoxIconType::NoIcon);
                    aw->addTextEditor ("name", "My Preset");
                    aw->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
                    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
                    auto* awPtr = aw.get();
                    awPtr->enterModalState (true,
                        juce::ModalCallbackFunction::create (
                            [this, aw] (int res)
                            {
                                if (res == 1)
                                {
                                    const auto name = aw->getTextEditorContents ("name");
                                    if (audioProc.saveUserPreset (name))
                                        btnPresetName.setButtonText (name.toUpperCase());
                                }
                                aw->exitModalState (res);
                                aw->setVisible (false);
                            }), false);
                }
                else if (r == 2)
                {
                    audioProc.userPresetDirectory().revealToUser();
                }
                else if (r >= 1000 && r < 2000)
                {
                    const int idx = r - 1000;
                    if (idx < files.size() && audioProc.loadUserPresetFile (files[idx]))
                        btnPresetName.setButtonText (
                            files[idx].getFileNameWithoutExtension().toUpperCase());
                }
                else if (r >= 2000)
                {
                    const int idx = r - 2000;
                    if (idx < files.size())
                        files[idx].deleteFile();
                }
            });
    }

    WireframeEditor::~WireframeEditor()
    {
        presetBrowser.removeComponentListener (this);
        setLookAndFeel (nullptr);
    }

    void WireframeEditor::componentVisibilityChanged (juce::Component& c)
    {
        if (&c == &presetBrowser && ! presetBrowser.isVisible())
            browserBackdrop.setVisible (false);
    }

    //==========================================================================
    //  applyPreset — smooth-tween all params to preset values (250ms)
    //==========================================================================
    void WireframeEditor::applyPreset (int idx)
    {
        idx = juce::jlimit (0, bc2000dl::kNumPresets - 1, idx);
        const auto& p = bc2000dl::kPresets[idx];
        auto& v = audioProc.apvts;

        struct Tween { juce::String id; float fromN, toN; };
        auto targets = std::make_shared<std::vector<Tween>>();

        auto plan = [&] (const juce::String& id, float val)
        {
            if (auto* prm = v.getParameter (id))
            {
                const float toN   = prm->convertTo0to1 (val);
                const float fromN = prm->getValue();
                targets->push_back ({ id, fromN, toN });
            }
        };

        plan ("speed",              (float) p.speed);
        plan ("mic_gain",           p.mic_gain);
        plan ("mic_gain_r",         p.mic_gain_r);
        plan ("phono_gain",         p.phono_gain);
        plan ("phono_gain_r",       p.phono_gain_r);
        plan ("radio_gain",         p.radio_gain);
        plan ("radio_gain_r",       p.radio_gain_r);
        plan ("tape_formula",       (float) p.tape_formula);
        plan ("saturation_drive",   p.saturation_drive);
        plan ("saturation_drive_r", p.saturation_drive_r);
        plan ("bias_amount",        p.bias_amount);
        plan ("wow_flutter",        p.wow_flutter);
        plan ("multiplay_gen",      (float) p.multiplay_gen);
        plan ("bass_db",            p.bass_db);
        plan ("treble_db",          p.treble_db);
        plan ("balance",            p.balance);
        plan ("master_volume",      p.master_volume);
        plan ("echo_enabled",       p.echo_enabled ? 1.0f : 0.0f);
        plan ("echo_amount",        p.echo_amount);
        plan ("echo_amount_r",      p.echo_amount_r);
        plan ("bypass_tape",        0.0f);
        plan ("speaker_monitor",    0.0f);
        plan ("synchroplay",        0.0f);
        plan ("pause",              0.0f);
        plan ("sos_enabled",        0.0f);
        plan ("pa_enabled",         0.0f);
        plan ("mic_loz",            0.0f);

        // 250 ms smoothstep tween (15 frames @ ~60Hz)
        constexpr int kFrames = 15;
        for (int f = 1; f <= kFrames; ++f)
        {
            const float t  = (float) f / (float) kFrames;
            const float e  = t * t * (3.0f - 2.0f * t);
            const bool last = (f == kFrames);
            juce::Timer::callAfterDelay (16 * f, [this, targets, e, last]
            {
                for (const auto& tw : *targets)
                    if (auto* prm = audioProc.apvts.getParameter (tw.id))
                    {
                        const float val = last
                            ? tw.toN
                            : juce::jlimit (0.0f, 1.0f,
                                tw.fromN + (tw.toN - tw.fromN) * e);
                        prm->setValueNotifyingHost (val);
                    }
            });
        }
    }

    //==========================================================================
    //  resized() — positions ALL controls per wireframe SVG coordinates
    //==========================================================================
    void WireframeEditor::resized()
    {
        // ----- Top deck zone -----
        reelDeck.setBounds (60, 48, 800, 310);
        cbSpeed.setBounds  (440, 175, 40, 16);

        // ----- Premium preset browser — centered modal overlay -----
        // Backdrop fyller editorn (dimmar appen); browsern sitter centrerad
        // ovanpå.  680×520 lämnar ~120 px sidomarginal + ~130 px topp/botten
        // så appen syns tydligt runtom — täcker inte hela fönstret.
        browserBackdrop.setBounds (getLocalBounds());
        {
            const int bw = 680;
            const int bh = 520;
            presetBrowser.setBounds ((kW - bw) / 2, (kH - bh) / 2, bw, bh);
        }

        // ----- Preset bar (left section of brushed alu title strip) -----
        btnPresetPrev.setBounds (70,  362, 22, 20);
        btnPresetName.setBounds (96, 362, 140, 20);
        btnPresetNext.setBounds (240, 362, 22, 20);
        // A/B-compare-kluster på HÖGER sida (preset-bar vänster, A/B höger →
        // varumärket äkta centrerat mellan dem). Slutar @816, power-LED @838.
        btnAbA   .setBounds (755, 362, 19, 20);
        btnAbB   .setBounds (776, 362, 19, 20);
        btnAbCopy.setBounds (797, 362, 19, 20);

        // ----- 9 service-knobs + 2 combos (y=406-430) — auth-restoration -----
        // Spacing: 75 px (was 90 for 8 items). Total 60-860 with 60 padding each side.
        const int knobSize  = 22;
        const int knobYbase = 406;
        const int slotX[11] = { 110, 185, 260, 335, 410, 485, 560, 635, 710, 785, 830 };
        knobBiasL      .setBounds (slotX[0] - knobSize/2, knobYbase, knobSize, knobSize);
        knobBiasR      .setBounds (slotX[1] - knobSize/2, knobYbase, knobSize, knobSize);
        knobSatL       .setBounds (slotX[2] - knobSize/2, knobYbase, knobSize, knobSize);
        knobSatR       .setBounds (slotX[3] - knobSize/2, knobYbase, knobSize, knobSize);
        knobWow        .setBounds (slotX[4] - knobSize/2, knobYbase, knobSize, knobSize);
        knobPrintTh    .setBounds (slotX[5] - knobSize/2, knobYbase, knobSize, knobSize);
        knobStereoAsym .setBounds (slotX[6] - knobSize/2, knobYbase, knobSize, knobSize);
        knobMultiplay  .setBounds (slotX[7] - knobSize/2, knobYbase, knobSize, knobSize);
        knobMainsHum   .setBounds (slotX[8] - knobSize/2, knobYbase, knobSize, knobSize);
        // TAPE FORMULA combo — wider for "SCOTCH" to fit
        cbTapeFormula  .setBounds (slotX[9] - 30,         knobYbase + 2,  60, 20);
        // MAINS HUM freq combo — wider for "50 Hz" / "60 Hz" to fit
        cbMainsHumFreq .setBounds (slotX[10] - 24,        knobYbase + 2,  48, 20);

        // ----- Vänster zon — rockers -----
        // Row A: 6 buttons at y=468-480
        const int rockerH = 12;
        const int rockerW = 22;
        btnTrack1   .setBounds (100, 468, rockerW, rockerH);
        btnTrack2   .setBounds (130, 468, rockerW, rockerH);
        btnRecArm1  .setBounds (160, 468, rockerW, rockerH);
        btnRecArm2  .setBounds (190, 468, rockerW, rockerH);
        btnSync     .setBounds (220, 468, rockerW, rockerH);
        btnMoment   .setBounds (250, 468, rockerW, rockerH);
        // Row B: PA + Förstärkare (centered in left zone)
        btnPA         .setBounds (155, 513, 32, rockerH);
        btnForstarkare.setBounds (199, 513, 38, rockerH);
        // Row C: Monitor + SoS
        cbMonitor   .setBounds (95, 598, 70, rockerH);
        btnSoSOn    .setBounds (224, 598, 22, rockerH);  // centered under S on S header (235)
        // Row D: Speaker
        btnSpkInt   .setBounds (100, 647, rockerW, rockerH);
        btnSpkExt   .setBounds (126, 647, rockerW, rockerH);
        btnSpkMute  .setBounds (152, 647, rockerW, rockerH);
        // 4 narrow horizontal Kyoritsu VUs in a row — IN L · IN R · OUT L · OUT R
        // Photo-accurate: very thin horizontal strips (Beocord 2400 style)
        // Zone is 90-300 (210 wide). 4 × 50 + 3 × 3 (gaps) = 209
        {
            const int vuW = 50, vuH = 22, vuY = 675, gap = 3;
            int x = 96;
            vuInL .setBounds (x, vuY, vuW, vuH); x += vuW + gap;
            vuInR .setBounds (x, vuY, vuW, vuH); x += vuW + gap;
            vuOutL.setBounds (x, vuY, vuW, vuH); x += vuW + gap;
            vuOutR.setBounds (x, vuY, vuW, vuH);
        }

        // ----- 10 faders (centerx, y=480-660, length 180) -----
        const int faderW = 14;
        const int faderH = 180;
        const int fy     = 480;
        const int faderXs[10] = { 328, 342, 383, 397, 438, 452, 493, 507, 548, 562 };
        faderRadioL .setBounds (faderXs[0] - faderW/2, fy, faderW, faderH);
        faderRadioR .setBounds (faderXs[1] - faderW/2, fy, faderW, faderH);
        faderPhonoL .setBounds (faderXs[2] - faderW/2, fy, faderW, faderH);
        faderPhonoR .setBounds (faderXs[3] - faderW/2, fy, faderW, faderH);
        faderMicL   .setBounds (faderXs[4] - faderW/2, fy, faderW, faderH);
        faderMicR   .setBounds (faderXs[5] - faderW/2, fy, faderW, faderH);
        faderEchoL  .setBounds (faderXs[6] - faderW/2, fy, faderW, faderH);
        faderEchoR  .setBounds (faderXs[7] - faderW/2, fy, faderW, faderH);
        faderMasterL.setBounds (faderXs[8] - faderW/2, fy, faderW, faderH);
        faderMasterR.setBounds (faderXs[9] - faderW/2, fy, faderW, faderH);

        // ----- Fader tabs (y=674, centerx per source) — only 3 active modes -----
        const int tabW = 22, tabH = 11;  // wider for "HiZ" text on mic mode
        cbRadioMode .setBounds (335 - tabW/2, 674, tabW, tabH);
        cbPhonoMode .setBounds (390 - tabW/2, 674, tabW, tabH);
        cbMicMode   .setBounds (445 - tabW/2, 674, tabW, tabH);

        // ----- Höger zon — UAD-grade aligned column layout -----
        // 2 columns: LEFT x=660 (INPUT, BASS, ECHO TIME), RIGHT x=740 (OUTPUT, TREBLE, ECHO FB)
        // All knobs same size 44×44. Echo ON centered between columns.
        // Sub-panel: y=456-580.  Header at y=460 (h=12 → ends y=472).
        // INPUT/OUTPUT-knobs flyttade 6 px upp (492 → 486) + BASS/TREBLE 4 px ner
        // (544 → 548) så label-luckan mellan raderna växte från 8 → 18 px.
        // Behövs för att 8.5 pt INPUT/OUTPUT-label ska få plats utan knob-överlapp.
        // Topprad: 4 plugin-utility-rattar symmetriskt kring panelmitten x=704,
        // 56 px delning (centers 620/676/732/788) → 12 px lucka mellan 44-px-knobs.
        knobInputTrim .setBounds (620 - 22, 486 - 22, 44, 44);  // 464-508
        knobOutputTrim.setBounds (676 - 22, 486 - 22, 44, 44);
        knobMix       .setBounds (732 - 22, 486 - 22, 44, 44);
        knobTapeNoise .setBounds (788 - 22, 486 - 22, 44, 44);
        // Bass + Treble — y=548 center (range 526-570, ends within panel y=580)
        // Ton-trio: BASS · BALANCE · TREBLE (3 knobs i bottenraden, inom panelen 594–814)
        knobBass   .setBounds (648 - 22, 548 - 22, 44, 44);
        knobBalance.setBounds (704 - 22, 548 - 22, 44, 44);
        knobTreble .setBounds (760 - 22, 548 - 22, 44, 44);
        // Echo ON toggle — centered between columns (x=700), inside sub-panel
        // (under ECHO header at y=602; knobs below at y=645)
        tEchoPluginOn.setBounds (700 - 16, 616, 32, 14);
        // Echo TIME / FB — now 44×44, column-aligned with knobs above
        knobEchoTime.setBounds (660 - 22, 645 - 22, 44, 44);
        knobEchoFb  .setBounds (740 - 22, 645 - 22, 44, 44);
    }

    //==========================================================================
    //  paint() — static wireframe line-art (case, panels, reels static elements)
    //==========================================================================
    void WireframeEditor::paint (juce::Graphics& g)
    {
        TRACE_COMPONENT();
        // ===== Background (subtle dark studio surface) =====
        g.fillAll (juce::Colour (0xFF2A1E14));

        // Photoreal colour palette
        const juce::Colour stroke      (LnF::kStroke);
        const juce::Colour blackPanel  (LnF::kPanelBlk);
        const juce::Colour silk        (LnF::kSilk);
        const juce::Colour silkDim     (LnF::kSilkDim);

        // ===== Engraved-text helper — UAD-style "printed/engraved into panel" =====
        // 3-layer: dark shadow below + body + thin highlight above for embossed look
        auto drawEngraved = [&] (const juce::String& text, juce::Rectangle<int> bounds,
                                  juce::Colour body, juce::Font font,
                                  juce::Justification just = juce::Justification::centred)
        {
            g.setFont (font);
            // Dark drop-shadow (1 px below) — engraved-into-metal feel
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.drawText (text, bounds.translated (0, 1), just, false);
            // Subtle bright top-edge highlight (catches studio light) — emboss
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            g.drawText (text, bounds.translated (0, -1), just, false);
            // Body text
            g.setColour (body);
            g.drawText (text, bounds, just, false);
        };

        // ===== Wooden case — procedural walnut gradient (bitmap stretch distorts) =====
        {
            juce::Rectangle<float> woodR (40.0f, 30.0f, 840.0f, 720.0f);

            // ----- Ambient studio glow — soft warm halo around case edges -----
            // Subtle "powered on" feel: warm light bleeding outward, like the
            // case is sitting on a softly-lit studio desk.  Radial gradient,
            // anchored to case bounds, extending ~30 px beyond.
            {
                const auto cx = woodR.getCentreX();
                const auto cy = woodR.getCentreY();
                const float rOuter = juce::jmax (woodR.getWidth(), woodR.getHeight()) * 0.62f;
                juce::ColourGradient glow (juce::Colour (0xFF3A1F0E).withAlpha (0.0f), cx, cy,
                                              juce::Colour (0xFF1A0A04).withAlpha (0.0f),
                                              cx + rOuter, cy, true);
                glow.addColour (0.65, juce::Colour (0xFF5A3018).withAlpha (0.18f));
                glow.addColour (0.85, juce::Colour (0xFF2A1408).withAlpha (0.10f));
                g.setGradientFill (glow);
                g.fillRect (getLocalBounds());
            }

            // Cabinet drop shadow on background (melatonin cached)
            juce::Path casePath;
            casePath.addRoundedRectangle (woodR, 8.0f);
            caseShadow.render (g, casePath);

            juce::ColourGradient grad (juce::Colour (LnF::kWoodHi), woodR.getX(), woodR.getY(),
                                        juce::Colour (LnF::kWoodBot), woodR.getX(), woodR.getBottom(),
                                        false);
            grad.addColour (0.15, juce::Colour (LnF::kWoodTop));
            grad.addColour (0.50, juce::Colour (0xFFB06030));
            grad.addColour (0.85, juce::Colour (0xFF7A3818));
            g.setGradientFill (grad);
            g.fillRoundedRectangle (woodR, 8.0f);

            // ===== Authentic HORIZONTAL wood grain (matches Beocord 2400 photo) =====
            // Teak has long horizontal grain — multiple soft streaks at varying alpha
            juce::Random rng (54321);
            for (int i = 0; i < 80; ++i)
            {
                const float gy = 35.0f + rng.nextFloat() * 712.0f;
                const float alpha = 0.04f + rng.nextFloat() * 0.10f;
                const float lineThickness = 0.3f + rng.nextFloat() * 0.5f;
                const bool darker = rng.nextBool();
                g.setColour (darker
                    ? juce::Colour::fromFloatRGBA (0.0f, 0.0f, 0.0f, alpha)
                    : juce::Colour::fromFloatRGBA (1.0f, 0.85f, 0.65f, alpha * 0.6f));
                // Slight horizontal undulation in grain
                const float xStart = 44.0f + rng.nextFloat() * 4.0f;
                const float xEnd   = 876.0f - rng.nextFloat() * 4.0f;
                const float yWave  = (rng.nextFloat() - 0.5f) * 1.5f;
                g.drawLine (xStart, gy, xEnd, gy + yWave, lineThickness);
            }
            // A few more pronounced grain "knots" (darker spots)
            for (int i = 0; i < 12; ++i)
            {
                const float kx = 80.0f + rng.nextFloat() * 760.0f;
                const float ky = 50.0f + rng.nextFloat() * 680.0f;
                g.setColour (juce::Colour::fromFloatRGBA (0.0f, 0.0f, 0.0f, 0.15f));
                g.fillEllipse (kx, ky, 18.0f + rng.nextFloat() * 8.0f,
                                3.0f + rng.nextFloat() * 1.5f);
            }

            // Edge wear: subtle darker line where wood meets the inset panel (realistic aging)
            g.setColour (juce::Colour (0x33000000));
            g.drawRoundedRectangle (woodR.reduced (3.5f), 6.0f, 0.5f);
            // Top-edge subtle finish-line (where lacquer reflects studio light)
            g.setColour (juce::Colour (0x22FFFFFF));
            g.drawLine (woodR.getX() + 8, woodR.getY() + 4,
                         woodR.getRight() - 8, woodR.getY() + 4, 0.4f);
            // Microscopic dust/fingerprint imperfections (super subtle)
            {
                juce::Random impRng (98765);
                for (int i = 0; i < 24; ++i)
                {
                    const float ix = woodR.getX() + 10 + impRng.nextFloat() * (woodR.getWidth() - 20);
                    const float iy = woodR.getY() + 10 + impRng.nextFloat() * (woodR.getHeight() - 20);
                    const float r = 0.5f + impRng.nextFloat() * 1.2f;
                    g.setColour (juce::Colour::fromFloatRGBA (0, 0, 0, 0.04f));
                    g.fillEllipse (ix, iy, r, r * 0.6f);
                }
            }
            // Inner highlight (top edge of wood)
            g.setColour (juce::Colour (0x66FFFFFF));
            g.drawRoundedRectangle (woodR.reduced (1.5f), 7.0f, 0.6f);
            // Outer shadow ring
            g.setColour (juce::Colour (0xCC000000));
            g.drawRoundedRectangle (woodR, 8.0f, 1.2f);
        }

        auto drawBlackPanel = [&] (float x, float y, float w, float h, float corner)
        {
            juce::Rectangle<float> r (x, y, w, h);
            juce::ColourGradient grad (juce::Colour (LnF::kPanelBlkHi), x, y,
                                        juce::Colour (LnF::kPanelBlkLo), x, y + h,
                                        false);
            grad.addColour (0.5, blackPanel);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (r, corner);
            // Thin specular highlight at top
            g.setColour (juce::Colour (0x40FFFFFF));
            g.drawLine (x + 4, y + 0.5f, x + w - 4, y + 0.5f, 0.7f);
            // Inner shadow — recessed-into-cabinet feel
            juce::Path pp;
            pp.addRoundedRectangle (r, corner);
            panelInnerShadow.render (g, pp);
            // Bevel
            g.setColour (juce::Colour (0xAA000000));
            g.drawRoundedRectangle (r, corner, 1.0f);
        };

        // ===== Top deck panel (black) =====
        drawBlackPanel (60.0f, 48.0f, 800.0f, 310.0f, 2.0f);

        // ===== Drop shadows under the reels — UAD-grade depth (melatonin cached) =====
        // Rendered BEFORE reel-deck Component paints; reel bitmaps then composite on top.
        {
            juce::Path reelL, reelR;
            reelL.addEllipse (juce::Rectangle<float> (60 + 220 - 130, 48 + 147 - 130, 260.0f, 260.0f));
            reelR.addEllipse (juce::Rectangle<float> (60 + 580 - 130, 48 + 147 - 130, 260.0f, 260.0f));
            reelShadow.render (g, reelL);
            reelShadow.render (g, reelR);
        }

        // ===== Tension rollers (chrome posts) =====
        auto drawChromePost = [&] (float cx, float cy)
        {
            juce::ColourGradient gr (juce::Colour (LnF::kChromeHi), cx - 8, cy - 12,
                                      juce::Colour (LnF::kChromeLo), cx + 8, cy + 12, false);
            g.setGradientFill (gr);
            g.fillEllipse (cx - 12, cy - 12, 24, 24);
            g.setColour (juce::Colour (LnF::kStroke));
            g.drawEllipse (cx - 12, cy - 12, 24, 24, 1.0f);
            g.setColour (juce::Colour (0xFF1A1A1A));
            g.fillEllipse (cx - 3, cy - 3, 6, 6);
            g.setColour (juce::Colour (0x88FFFFFF));
            g.fillEllipse (cx - 9, cy - 11, 5, 3);
        };
        drawChromePost (95,  280);
        drawChromePost (825, 280);

        // ===== Counter (5) — vintage 7-segment amber display =====
        // Period-correct: 1968 tape decks had Nixie tubes or 7-segment LED displays.
        // Uses fontaudio Digital0-9 glyphs with ghost-segment "8888" backdrop for
        // authentic dim-segments-when-unlit look (the LCD/Nixie signature).
        {
            juce::Rectangle<float> counter (430, 62, 60, 20);
            g.setColour (juce::Colour (0xFF0A0807));   // deep amber-black (LCD off-state)
            g.fillRoundedRectangle (counter, 2.0f);

            // Subtle inner vertical gradient (LCD glass feel)
            juce::ColourGradient lcdGr (juce::Colour (0xFF1A1410), counter.getX(), counter.getY(),
                                          juce::Colour (0xFF050302), counter.getX(), counter.getBottom(), false);
            g.setGradientFill (lcdGr);
            g.fillRoundedRectangle (counter.reduced (0.8f), 1.5f);

            // Recessed inner shadow (LCD-like inset)
            juce::Path cp;
            cp.addRoundedRectangle (counter, 2.0f);
            counterInnerShadow.render (g, cp);

            // Per-digit slot geometry — 4 digits across the counter width
            constexpr float kDigitGlyphSize = 13.0f;     // 7-segment glyph height
            const float slotW = (counter.getWidth() - 4.0f) / 4.0f;   // 4 equal columns

            for (int i = 0; i < 4; ++i)
            {
                const float slotX = counter.getX() + 2.0f + (float) i * slotW;
                const juce::Rectangle<float> slot (slotX, counter.getY() + 3.0f, slotW, counter.getHeight() - 6.0f);

                // Ghost "8" — all segments dimly lit (classic Nixie/LCD off-state)
                icons::drawIcon (g, icons::Digital8, slot,
                                  juce::Colour (0xFF3A1F08).withAlpha (0.55f),
                                  kDigitGlyphSize);

                // Active digit — bright amber glow
                const juce::String& glyph = (i < counterText.length())
                    ? icons::digitGlyph ((char) counterText[i])
                    : icons::Digital0;

                // Soft glow halo (offset shadow for emission look)
                icons::drawIcon (g, glyph, slot.translated (0.0f, 0.0f),
                                  juce::Colour (0xFFFFC080).withAlpha (0.25f),
                                  kDigitGlyphSize + 2.0f);
                // Crisp digit body
                icons::drawIcon (g, glyph, slot,
                                  juce::Colour (0xFFFFD080),    // warm amber emission
                                  kDigitGlyphSize);
            }

            // Vertical dividers between digits — thin tube-separator lines
            g.setColour (juce::Colour (0xFF222220));
            for (int i = 1; i < 4; ++i)
            {
                const float dx = counter.getX() + 2.0f + (float) i * slotW;
                g.drawLine (dx, counter.getY() + 2, dx, counter.getBottom() - 2, 0.4f);
            }

            // Outer bezel
            g.setColour (juce::Colour (LnF::kStroke));
            g.drawRoundedRectangle (counter, 2.0f, 0.8f);
            // Bezel highlight
            g.setColour (juce::Colour (0x88FFFFFF));
            g.drawLine (counter.getX() + 1, counter.getY() + 0.5f,
                         counter.getRight() - 1, counter.getY() + 0.5f, 0.5f);
        }
        // Counter label — engraved
        drawEngraved ("COUNTER",
                       juce::Rectangle<int> (430, 84, 60, 8),
                       silkDim,
                       juce::Font (juce::FontOptions (6.5f, juce::Font::bold))
                            .withExtraKerningFactor (0.22f));

        // ===== Manöverspak (1) — beefier chrome transport lever (per Beocord 2400 photo) =====
        {
            const float leverX     = 820.0f;
            const float shaftTop   = 155.0f;   // taller now
            const float shaftBot   = 225.0f;   // 70 px shaft (was 40)
            const float shaftWidth = 5.0f;     // wider (was 3.6)
            const float ballR      = 9.0f;     // bigger ball (was 5)
            const float baseW      = 16.0f;    // chrome pivot-housing at bottom
            const float baseH      = 8.0f;

            // ===== Combined drop shadow for whole lever =====
            {
                juce::Path leverPath;
                leverPath.addRoundedRectangle (leverX - shaftWidth/2, shaftTop,
                                                 shaftWidth, shaftBot - shaftTop, 2.0f);
                leverPath.addEllipse (leverX - ballR, shaftTop - ballR * 2, ballR * 2, ballR * 2);
                leverPath.addRoundedRectangle (leverX - baseW/2, shaftBot - baseH/2,
                                                 baseW, baseH, 3.0f);
                manoverspakShadow.render (g, leverPath);
            }

            // ===== Chrome pivot-housing at the base (cylindrical chrome boot) =====
            juce::Rectangle<float> base (leverX - baseW/2, shaftBot - baseH/2, baseW, baseH);
            juce::ColourGradient baseGr (juce::Colour (0xFFE8E8E8), base.getX(), base.getY(),
                                           juce::Colour (0xFF5E5E5E), base.getX(), base.getBottom(), false);
            baseGr.addColour (0.3, juce::Colour (0xFFD0D0D0));
            baseGr.addColour (0.6, juce::Colour (0xFF888888));
            g.setGradientFill (baseGr);
            g.fillRoundedRectangle (base, 3.0f);
            // Top edge highlight
            g.setColour (juce::Colour (0xCCFFFFFF));
            g.drawLine (base.getX() + 2, base.getY() + 1, base.getRight() - 2, base.getY() + 1, 0.6f);
            // Outline
            g.setColour (juce::Colour (LnF::kStroke));
            g.drawRoundedRectangle (base, 3.0f, 0.6f);

            // ===== Chrome shaft — beefier with horizontal gradient =====
            juce::ColourGradient shaftGr (juce::Colour (0xFF4E4E4E), leverX - shaftWidth/2, shaftTop,
                                            juce::Colour (0xFF8E8E8E), leverX, shaftTop, false);
            shaftGr.addColour (0.5, juce::Colour (0xFFF8F8F8));
            shaftGr.point2 = { leverX + shaftWidth/2, shaftTop };
            g.setGradientFill (shaftGr);
            g.fillRoundedRectangle (leverX - shaftWidth/2, shaftTop, shaftWidth, shaftBot - shaftTop, 2.0f);
            // Vertical highlight band (chrome reflection from above)
            g.setColour (juce::Colour (0xCCFFFFFF));
            g.drawLine (leverX - shaftWidth/2 + 1.0f, shaftTop + 1.5f,
                         leverX - shaftWidth/2 + 1.0f, shaftBot - 1.5f, 0.6f);
            // Vertical dark band on right (shadow side)
            g.setColour (juce::Colour (0x88000000));
            g.drawLine (leverX + shaftWidth/2 - 1.0f, shaftTop + 1.5f,
                         leverX + shaftWidth/2 - 1.0f, shaftBot - 1.5f, 0.5f);
            // Outline
            g.setColour (juce::Colour (LnF::kStroke));
            g.drawRoundedRectangle (leverX - shaftWidth/2, shaftTop, shaftWidth, shaftBot - shaftTop, 2.0f, 0.5f);

            // ===== Chrome ball at top — UAD-grade 8-layer =====
            // (1) Dark outer shadow ring around ball
            g.setColour (juce::Colour (0xCC000000));
            g.fillEllipse (leverX - ballR - 0.5f, shaftTop - ballR * 2 - 0.5f,
                            (ballR + 0.5f) * 2, (ballR + 0.5f) * 2);
            // (2) Body — 5-stop radial chrome
            juce::ColourGradient ballGr (juce::Colour (0xFFFFFFFF),
                                          leverX - ballR * 0.45f, shaftTop - ballR * 1.65f,
                                          juce::Colour (0xFF2E2E2E),
                                          leverX + ballR * 0.55f, shaftTop - ballR * 0.3f, true);
            ballGr.addColour (0.15, juce::Colour (0xFFE8E8E8));
            ballGr.addColour (0.40, juce::Colour (0xFFB0B0B0));
            ballGr.addColour (0.65, juce::Colour (0xFF707070));
            ballGr.addColour (0.85, juce::Colour (0xFF4A4A4A));
            g.setGradientFill (ballGr);
            g.fillEllipse (leverX - ballR, shaftTop - ballR * 2, ballR * 2, ballR * 2);
            // (3) Outline
            g.setColour (juce::Colour (LnF::kStroke));
            g.drawEllipse (leverX - ballR, shaftTop - ballR * 2, ballR * 2, ballR * 2, 0.7f);
            // (4) Bottom shadow arc (3D ball feel)
            g.setColour (juce::Colour (0x55000000));
            g.fillEllipse (leverX - ballR * 0.7f, shaftTop - ballR * 0.6f, ballR * 1.4f, ballR * 0.55f);
            // (5) Big soft specular highlight
            g.setColour (juce::Colour (0x77FFFFFF));
            g.fillEllipse (leverX - ballR * 0.65f, shaftTop - ballR * 1.85f, ballR * 0.9f, ballR * 0.45f);
            // (6) Medium specular
            g.setColour (juce::Colour (0xAAFFFFFF));
            g.fillEllipse (leverX - ballR * 0.48f, shaftTop - ballR * 1.72f, ballR * 0.55f, ballR * 0.25f);
            // (7) Tiny bright hot-spot
            g.setColour (juce::Colour (0xFFFFFFFF));
            g.fillEllipse (leverX - ballR * 0.35f, shaftTop - ballR * 1.62f, ballR * 0.22f, ballR * 0.12f);
            // (8) Rim edge highlight (top-left)
            g.setColour (juce::Colour (0x55FFFFFF));
            g.drawEllipse (leverX - ballR + 0.5f, shaftTop - ballR * 2 + 0.5f,
                            ballR * 2 - 1.0f, ballR * 2 - 1.0f, 0.4f);
        }

        // ===== Tape path guide pins — UAD-grade chrome (7 layers) =====
        auto drawGuide = [&] (float cx, float cy, float rr)
        {
            // (1) Soft drop shadow
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillEllipse (cx - rr - 0.3f, cy - rr + 0.8f, (rr + 0.3f) * 2, (rr + 0.3f) * 2);
            // (2) Outer dark rim (recessed feel)
            g.setColour (juce::Colour (0xFF080808));
            g.fillEllipse (cx - rr, cy - rr, rr * 2, rr * 2);
            // (3) Chrome body — radial 5-stop gradient
            juce::ColourGradient gr (juce::Colour (0xFFFFFFFF),
                                       cx - rr * 0.40f, cy - rr * 0.55f,
                                       juce::Colour (0xFF383838),
                                       cx + rr * 0.45f, cy + rr * 0.55f, true);
            gr.addColour (0.3, juce::Colour (0xFFE0E0E0));
            gr.addColour (0.7, juce::Colour (0xFF787878));
            g.setGradientFill (gr);
            g.fillEllipse (cx - rr + 0.5f, cy - rr + 0.5f,
                            (rr - 0.5f) * 2, (rr - 0.5f) * 2);
            // (4) Outer rim outline
            g.setColour (juce::Colour (LnF::kStroke));
            g.drawEllipse (cx - rr, cy - rr, rr * 2, rr * 2, 0.5f);
            // (5) Center pin-hole (dark inner dot)
            g.setColour (juce::Colour (0xFF0A0A0A));
            g.fillEllipse (cx - rr * 0.30f, cy - rr * 0.30f, rr * 0.6f, rr * 0.6f);
            // (6) Top-left specular highlight (chrome catch)
            g.setColour (juce::Colour (0xDDFFFFFF));
            g.fillEllipse (cx - rr * 0.50f, cy - rr * 0.65f, rr * 0.55f, rr * 0.30f);
            // (7) Tiny pinpoint hot-spot
            g.setColour (juce::Colour (0xFFFFFFFF));
            g.fillEllipse (cx - rr * 0.35f, cy - rr * 0.55f, rr * 0.18f, rr * 0.12f);
        };
        // Tape-path guide pins — moved closer to head assembly so they hug the
        // actual tape ribbon path instead of looking like random screws.
        drawGuide (398, 248, 4);   // left guide just before head
        drawGuide (522, 248, 4);   // right guide just after head

        // ============================================================
        //  Photoreal tape-head assembly — UAD-grade multi-layer rendering
        //  Matches B&O Beocord 2000 De Luxe service-manual schematic:
        //    mu-metal shield → chrome holder → 3 pole-pieces (E·R·P)
        //    with visible recording gaps → mounting screws in 4 corners.
        // ============================================================
        {
            // -------- Layer 1: drop shadow under entire assembly --------
            {
                juce::Path sp;
                sp.addRoundedRectangle (juce::Rectangle<float> (411, 248, 98, 34), 3.0f);
                bAndOPlaqueShadow.render (g, sp);
            }

            // -------- Layer 2: mu-metal mounting plate (matte black) --------
            const juce::Rectangle<float> shield (411, 248, 98, 34);
            juce::ColourGradient muGr (juce::Colour (0xFF2A2A2A), shield.getX(), shield.getY(),
                                         juce::Colour (0xFF0E0E0E), shield.getX(), shield.getBottom(), false);
            muGr.addColour (0.5, juce::Colour (0xFF181818));
            g.setGradientFill (muGr);
            g.fillRoundedRectangle (shield, 3.0f);

            // Subtle horizontal brushed texture on mu-metal
            g.setColour (juce::Colour (0x10FFFFFF));
            for (int yy = 2; yy < 32; yy += 3)
                g.drawLine (shield.getX() + 4, shield.getY() + (float) yy,
                             shield.getRight() - 4, shield.getY() + (float) yy, 0.2f);

            // Top edge highlight (matte sheen)
            g.setColour (juce::Colour (0x55FFFFFF));
            g.drawLine (shield.getX() + 3, shield.getY() + 0.7f,
                         shield.getRight() - 3, shield.getY() + 0.7f, 0.5f);
            // Bottom edge shadow
            g.setColour (juce::Colour (0xAA000000));
            g.drawLine (shield.getX() + 3, shield.getBottom() - 0.7f,
                         shield.getRight() - 3, shield.getBottom() - 0.7f, 0.5f);
            // Outline
            g.setColour (juce::Colour (LnF::kStroke));
            g.drawRoundedRectangle (shield, 3.0f, 0.6f);

            // -------- Layer 3: chrome head holder (the part the tape touches) --------
            const juce::Rectangle<float> chrome (419, 252, 82, 20);
            juce::ColourGradient chGr (juce::Colour (0xFFF0F0F0), chrome.getX(), chrome.getY(),
                                         juce::Colour (0xFF606060), chrome.getX(), chrome.getBottom(), false);
            chGr.addColour (0.35, juce::Colour (0xFFD8D8D8));
            chGr.addColour (0.65, juce::Colour (0xFF989898));
            g.setGradientFill (chGr);
            g.fillRoundedRectangle (chrome, 1.5f);

            // Brushed-chrome striations (horizontal)
            g.setColour (juce::Colour (0x14000000));
            for (int yy = 1; yy < 20; yy += 2)
                g.drawLine (chrome.getX() + 2, chrome.getY() + (float) yy,
                             chrome.getRight() - 2, chrome.getY() + (float) yy, 0.25f);

            // Top bevel highlight (where tape rides)
            g.setColour (juce::Colour (0xEEFFFFFF));
            g.drawLine (chrome.getX() + 2, chrome.getY() + 0.7f,
                         chrome.getRight() - 2, chrome.getY() + 0.7f, 0.7f);
            // Bottom bevel shadow
            g.setColour (juce::Colour (0xCC000000));
            g.drawLine (chrome.getX() + 2, chrome.getBottom() - 0.7f,
                         chrome.getRight() - 2, chrome.getBottom() - 0.7f, 0.7f);
            g.setColour (juce::Colour (LnF::kStroke));
            g.drawRoundedRectangle (chrome, 1.5f, 0.4f);

            // -------- Layer 4: 3 head pole-pieces (E · R · P) with gaps --------
            // Pole positions on chrome: 22px wide each, 4px spacing between
            const float poleW   = 18.0f;
            const float poleH   = 14.0f;
            const float poleY   = chrome.getY() + 3.0f;
            const float startX  = chrome.getCentreX() - (poleW * 1.5f + 4.0f);

            for (int i = 0; i < 3; ++i)
            {
                const float px = startX + (float) i * (poleW + 4.0f);
                juce::Rectangle<float> pole (px, poleY, poleW, poleH);

                // Pole body — darker chrome to contrast against bright holder
                juce::ColourGradient poleGr (juce::Colour (0xFF8A8A8A), pole.getX(), pole.getY(),
                                              juce::Colour (0xFF383838), pole.getX(), pole.getBottom(), false);
                poleGr.addColour (0.5, juce::Colour (0xFF606060));
                g.setGradientFill (poleGr);
                g.fillRoundedRectangle (pole, 1.0f);

                // Top pole-tip — bright polished face (where tape contacts)
                juce::ColourGradient tipGr (juce::Colour (0xFFE8E8E8), pole.getX(), pole.getY(),
                                             juce::Colour (0xFF989898), pole.getX(), pole.getY() + 3.0f, false);
                g.setGradientFill (tipGr);
                g.fillRoundedRectangle (pole.withHeight (3.0f), 1.0f);

                // Recording gap — thin vertical dark line down the centre
                g.setColour (juce::Colour (0xFF050505));
                const float gapX = pole.getCentreX();
                g.drawLine (gapX, pole.getY() + 0.5f, gapX, pole.getBottom() - 0.5f, 0.6f);
                // Gap highlight (one side of gap catches light)
                g.setColour (juce::Colour (0x44FFFFFF));
                g.drawLine (gapX - 0.5f, pole.getY() + 0.5f, gapX - 0.5f, pole.getBottom() - 0.5f, 0.3f);

                // Pole outline
                g.setColour (juce::Colour (0xCC000000));
                g.drawRoundedRectangle (pole, 1.0f, 0.4f);
            }

            // -------- Layer 5: 4 mounting screws on mu-metal corners --------
            auto drawTinyScrew = [&] (float cx, float cy)
            {
                // Shadow well
                g.setColour (juce::Colours::black.withAlpha (0.55f));
                g.fillEllipse (cx - 2.0f, cy - 1.7f, 4.0f, 4.0f);
                // Recessed dark hole
                g.setColour (juce::Colour (0xFF0A0A0A));
                g.fillEllipse (cx - 1.7f, cy - 1.7f, 3.4f, 3.4f);
                // Chrome screw head
                juce::ColourGradient sgr (juce::Colour (0xFFE8E8E8), cx - 1.2f, cy - 1.4f,
                                            juce::Colour (0xFF606060), cx + 1.2f, cy + 1.4f, false);
                g.setGradientFill (sgr);
                g.fillEllipse (cx - 1.3f, cy - 1.3f, 2.6f, 2.6f);
                // Cross slot
                g.setColour (juce::Colour (0xFF101010));
                g.drawLine (cx - 1.0f, cy, cx + 1.0f, cy, 0.4f);
                g.drawLine (cx, cy - 1.0f, cx, cy + 1.0f, 0.4f);
                // Tiny highlight on screw head
                g.setColour (juce::Colour (0xCCFFFFFF));
                g.fillEllipse (cx - 0.8f, cy - 0.9f, 0.8f, 0.4f);
            };
            drawTinyScrew (shield.getX() + 4,    shield.getY() + 4);
            drawTinyScrew (shield.getRight() - 4, shield.getY() + 4);
            drawTinyScrew (shield.getX() + 4,    shield.getBottom() - 4);
            drawTinyScrew (shield.getRight() - 4, shield.getBottom() - 4);
        }

        // 2-TRACK / 4-TRACK — engraved label above head assembly
        drawEngraved ("2-TRACK / 4-TRACK",
                       juce::Rectangle<int> (411, 238, 98, 9),
                       silkDim,
                       juce::Font (juce::FontOptions (7.0f, juce::Font::bold))
                            .withExtraKerningFactor (0.12f));

        // B&O logo plaque removed — area cleaned up to let the photoreal
        // tape-head assembly above stand alone as the focal point.

        // ===== Hastighet/speed knob (28) — vertical chrome cylinder per Beocord 2400 photo =====
        {
            // Vertical cylindrical chrome knob (taller than wide)
            const float cyX = 460.0f, cyY = 150.0f;
            const float cyW = 18.0f, cyH = 32.0f;
            juce::Rectangle<float> cyl (cyX - cyW/2, cyY - cyH/2, cyW, cyH);

            // Melatonin-cached drop shadow (cleaner than procedural)
            juce::Path cylPath;
            cylPath.addRoundedRectangle (cyl, 3.0f);
            speedKnobShadow.render (g, cylPath);

            // Body: vertical chrome gradient (light center, darker edges)
            juce::ColourGradient cylGr (juce::Colour (0xFFB8B8B8), cyX - cyW/2, cyY,
                                          juce::Colour (0xFFE8E8E8), cyX, cyY, false);
            cylGr.addColour (0.5, juce::Colour (0xFFF8F8F8));
            cylGr.addColour (0.8, juce::Colour (0xFFA8A8A8));
            cylGr.point2 = { cyX + cyW/2, cyY };
            g.setGradientFill (cylGr);
            g.fillRoundedRectangle (cyl, 3.0f);

            // Horizontal scale lines on the cylinder (rotation indicators)
            g.setColour (juce::Colour (0x99000000));
            for (int i = 1; i <= 5; ++i)
            {
                const float ly = cyY - cyH/2 + (float) i * (cyH / 6.0f);
                g.drawLine (cyX - cyW/2 + 2, ly, cyX + cyW/2 - 2, ly, 0.5f);
            }

            // Top + bottom edge highlights (cylinder caps)
            g.setColour (juce::Colour (0xCCFFFFFF));
            g.drawLine (cyX - cyW/2 + 2, cyY - cyH/2 + 1, cyX + cyW/2 - 2, cyY - cyH/2 + 1, 0.7f);
            g.setColour (juce::Colour (0x77000000));
            g.drawLine (cyX - cyW/2 + 2, cyY + cyH/2 - 1, cyX + cyW/2 - 2, cyY + cyH/2 - 1, 0.7f);

            // Outer outline
            g.setColour (juce::Colour (LnF::kStroke));
            g.drawRoundedRectangle (cyl, 3.0f, 0.7f);
        }

        // ===== UAD-grade tape-guide-posts (chrome cylinders) — 8 layer detail =====
        auto drawTapeGuide = [&] (float cx, float cy)
        {
            // (1) Multi-stack soft drop shadow
            for (int i = 2; i >= 1; --i)
            {
                const float a = 0.20f / (float) i;
                g.setColour (juce::Colour::fromFloatRGBA (0, 0, 0, a));
                g.fillEllipse (cx - 3.5f - (float) i * 0.3f, cy - 3.0f + (float) i * 0.4f,
                                (3.5f + (float) i * 0.3f) * 2.0f, (3.0f + (float) i * 0.3f) * 2.0f);
            }
            // (2) Outer dark ring (recessed feel)
            g.setColour (juce::Colour (0xFF0A0A0A));
            g.fillEllipse (cx - 3.3f, cy - 3.3f, 6.6f, 6.6f);
            // (3) Chrome cylinder — 5-stop radial gradient
            juce::ColourGradient gp (juce::Colour (0xFFFFFFFF), cx - 0.5f, cy - 1.5f,
                                      juce::Colour (0xFF3E3E3E), cx + 2.0f, cy + 2.0f, true);
            gp.addColour (0.4, juce::Colour (0xFFD0D0D0));
            gp.addColour (0.75, juce::Colour (0xFF888888));
            g.setGradientFill (gp);
            g.fillEllipse (cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
            // (4) Concentric brushed ring (subtle)
            g.setColour (juce::Colour (0x22000000));
            g.drawEllipse (cx - 2.0f, cy - 2.0f, 4.0f, 4.0f, 0.3f);
            g.setColour (juce::Colour (0x33FFFFFF));
            g.drawEllipse (cx - 1.8f, cy - 1.8f, 3.6f, 3.6f, 0.25f);
            // (5) Inner darker depression (top of post)
            g.setColour (juce::Colour (0xFF202020));
            g.fillEllipse (cx - 0.9f, cy - 0.9f, 1.8f, 1.8f);
            // (6) Outline
            g.setColour (juce::Colour (LnF::kStroke));
            g.drawEllipse (cx - 3.0f, cy - 3.0f, 6.0f, 6.0f, 0.4f);
            // (7) Top specular highlight (intense)
            g.setColour (juce::Colour (0xFFFFFFFF));
            g.fillEllipse (cx - 1.5f, cy - 2.6f, 2.5f, 1.2f);
            g.setColour (juce::Colour (0x88FFFFFF));
            g.fillEllipse (cx - 1.8f, cy - 2.8f, 3.0f, 1.0f);
        };
        // Positions: between reels on the tape path (below the head block)
        drawTapeGuide (398, 332);
        drawTapeGuide (522, 332);
        // Speed-ring position markings — engraved
        {
            const auto markFont = juce::Font (juce::FontOptions (6.5f, juce::Font::bold))
                                       .withExtraKerningFactor (0.10f);
            drawEngraved ("0",    juce::Rectangle<int> (415, 146, 10, 8), silk, markFont);
            drawEngraved ("4.75", juce::Rectangle<int> (414, 160, 18, 8), silk, markFont);
            drawEngraved ("9.5",  juce::Rectangle<int> (451, 117, 18, 8), silk, markFont);
            drawEngraved ("19",   juce::Rectangle<int> (490, 146, 10, 8), silk, markFont);
        }
        drawEngraved ("SPEED cm/s",
                       juce::Rectangle<int> (430, 197, 60, 9),
                       silkDim,
                       juce::Font (juce::FontOptions (7.5f, juce::Font::bold))
                            .withExtraKerningFactor (0.18f));

        // ===== Brushed aluminium title strip (with preset bar on left) =====
        {
            juce::Rectangle<float> strip (60, 358, 800, 28);

            // Drop shadow under strip (floats above panel — UAD/Soundtoys look)
            juce::Path stripPath;
            stripPath.addRectangle (strip);
            titleStripShadow.render (g, stripPath);

            juce::ColourGradient gr (juce::Colour (LnF::kAluHi), strip.getX(), strip.getY(),
                                      juce::Colour (LnF::kAluLo), strip.getX(), strip.getBottom(),
                                      false);
            gr.addColour (0.5, juce::Colour (0xFFC4BFB1));
            g.setGradientFill (gr);
            g.fillRect (strip);
            // Brushed pattern — faint horizontal lines
            g.setColour (juce::Colour (0x18000000));
            for (int yy = 0; yy < 28; yy += 2)
                g.drawLine (strip.getX(), strip.getY() + yy, strip.getRight(), strip.getY() + yy, 0.3f);
            // Top/bottom strokes
            g.setColour (juce::Colour (LnF::kStroke));
            g.drawLine (strip.getX(), strip.getY(),       strip.getRight(), strip.getY(), 0.8f);
            g.drawLine (strip.getX(), strip.getBottom() - 0.5f, strip.getRight(), strip.getBottom() - 0.5f, 0.8f);

            // ===== Preset-bar recessed well — UAD-style inset on brushed alu =====
            // Buttons (Prev/Name/Next) span x=70..262, y=362..382 → well at 66..266, 360..384
            {
                const juce::Rectangle<float> well (66, 360, 200, 24);

                // Inset dark fill — recessed feel against bright brushed alu
                juce::ColourGradient wellGr (juce::Colour (0xFF1A1A1A), well.getX(), well.getY(),
                                              juce::Colour (0xFF2E2E2E), well.getX(), well.getBottom(), false);
                wellGr.addColour (0.5, juce::Colour (0xFF202020));
                g.setGradientFill (wellGr);
                g.fillRoundedRectangle (well, 2.5f);

                // Cached inner shadow (re-uses panelInnerShadow for consistency)
                juce::Path wp;
                wp.addRoundedRectangle (well, 2.5f);
                panelInnerShadow.render (g, wp);

                // Inner highlight on bottom (light bouncing up from button face)
                g.setColour (juce::Colour (0x33FFFFFF));
                g.drawLine (well.getX() + 3, well.getBottom() - 0.6f,
                             well.getRight() - 3, well.getBottom() - 0.6f, 0.4f);
                // Outline
                g.setColour (juce::Colour (0xCC000000));
                g.drawRoundedRectangle (well, 2.5f, 0.7f);
            }

            // Section dividers: cappar preset-baren (vänster) och A/B (höger),
            // varumärket centrerat i mitten emellan.
            g.setColour (juce::Colour (0x40000000));
            g.drawLine (272, 363, 272, 381, 0.5f);   // efter preset-bar
            g.drawLine (746, 363, 746, 381, 0.5f);   // före A/B-kluster

            // (PRESET caption removed — was overlapping behind the button)

            // Brand — centrerat på SAMMA mittlinje (x=460) som raden under,
            // "TAPE MACHINE ALIGNMENT" (rect 60+800/2). Måste matcha den exakt,
            // annars ser varumärket osymmetriskt ut mot referensraden.
            // Preset-bar (slutar @262) och A/B (börjar @755) flankerar och clearas.
            constexpr int kBrandL = 60, kBrandW = 800;   // mitt = 460
            g.setColour (juce::Colour (LnF::kStroke));
            g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold))
                            .withExtraKerningFactor (0.28f));
            g.drawText ("GERMANIUM 2000 DELUXE",
                         juce::Rectangle<int> (kBrandL, 360, kBrandW, 16),
                         juce::Justification::centred, false);
            // UX: subtitle bumpad 6.5 → 8.5 + ökad kontrast (0xFF3A3A3A → 0xFF5A5A5A)
            // + rect-höjd 9 → 12.  Tidigare osynlig mot mörk bakgrund.
            g.setColour (juce::Colour (0xFF6A6A6A));
            g.setFont (juce::Font (juce::FontOptions (8.5f))
                            .withExtraKerningFactor (0.35f));
            g.drawText ("SOUNDBOYS TAPE EMULATION",
                         juce::Rectangle<int> (kBrandL, 373, kBrandW, 12),
                         juce::Justification::centred, false);

            // ===== Power-on LED — period-correct green dome at top-right of title strip =====
            // 1968 hi-fi convention: small green pilot lamp shows mains power on.
            // 7-layer rendering: well, dark recess, body gradient, outer glow halo,
            // specular hot-spot, outline.
            {
                const float lx = 838.0f, ly = strip.getCentreY();
                const float lr = 3.6f;
                // Soft outer green glow halo (radial)
                {
                    juce::ColourGradient gloHalo (juce::Colour (0xFF60FF60).withAlpha (0.55f), lx, ly,
                                                    juce::Colour (0xFF60FF60).withAlpha (0.0f),
                                                    lx + lr * 2.5f, ly, true);
                    g.setGradientFill (gloHalo);
                    g.fillEllipse (lx - lr * 2.5f, ly - lr * 2.5f, lr * 5.0f, lr * 5.0f);
                }
                // Recessed well shadow
                g.setColour (juce::Colour (0xFF050505));
                g.fillEllipse (lx - lr - 0.8f, ly - lr - 0.8f, (lr + 0.8f) * 2, (lr + 0.8f) * 2);
                // LED body — radial green gradient (light center, darker rim)
                {
                    juce::ColourGradient ledGr (juce::Colour (0xFFC8FFC8),
                                                  lx - lr * 0.3f, ly - lr * 0.45f,
                                                  juce::Colour (0xFF0A4A0A),
                                                  lx + lr * 0.5f, ly + lr * 0.5f, true);
                    ledGr.addColour (0.25, juce::Colour (0xFF50DD50));
                    ledGr.addColour (0.65, juce::Colour (0xFF1F8C1F));
                    g.setGradientFill (ledGr);
                    g.fillEllipse (lx - lr, ly - lr, lr * 2, lr * 2);
                }
                // Rim outline
                g.setColour (juce::Colour (0xFF030C03));
                g.drawEllipse (lx - lr, ly - lr, lr * 2, lr * 2, 0.5f);
                // Big soft specular highlight
                g.setColour (juce::Colour (0xCCFFFFFF));
                g.fillEllipse (lx - lr * 0.55f, ly - lr * 0.75f, lr * 0.7f, lr * 0.35f);
                // Tiny bright hot-spot
                g.setColour (juce::Colour (0xFFFFFFFF));
                g.fillEllipse (lx - lr * 0.35f, ly - lr * 0.62f, lr * 0.28f, lr * 0.16f);
            }
            // "POWER" engraved label below the LED
            drawEngraved ("POWER",
                           juce::Rectangle<int> (820, strip.getBottom() - 12, 36, 6),
                           juce::Colour (0xFF2A2820),
                           juce::Font (juce::FontOptions (5.5f, juce::Font::bold))
                                .withExtraKerningFactor (0.20f));
        }

        // ===== Service strip drop shadow (lifts dark strip above title) =====
        {
            juce::Path stripPath;
            stripPath.addRectangle (60.0f, 386.0f, 800.0f, 56.0f);
            serviceStripShadow.render (g, stripPath);
        }
        // ===== Service strip (procedural black panel — cleaner than stretched bitmap) =====
        drawBlackPanel (60.0f, 386.0f, 800.0f, 56.0f, 0.0f);
        // UX: header bumpad 8.0 → 10.5 + rect-höjd 10 → 13
        drawEngraved ("TAPE MACHINE ALIGNMENT",
                       juce::Rectangle<int> (60, 386, 800, 13),
                       silkDim,
                       juce::Font (juce::FontOptions (10.5f, juce::Font::bold))
                            .withExtraKerningFactor (0.30f));
        // Service-strip section glyph — Diskio (tape platter) flanking header
        {
            const auto glyphCol = juce::Colour (LnF::kSilkDim);
            const auto glyphSh  = juce::Colour (0xFF000000).withAlpha (0.55f);
            for (int dx : { 280, 620 })   // both sides of "TAPE MACHINE ALIGNMENT"
            {
                const auto r = juce::Rectangle<float> ((float) dx, 387.0f, 11.0f, 11.0f);
                icons::drawIcon (g, icons::Diskio, r.translated (0.4f, 0.4f), glyphSh, 10.0f);
                icons::drawIcon (g, icons::Diskio, r, glyphCol, 10.0f);
            }
        }
        // 9 knob slots + tape formula label — authentic-restoration labels
        const int slotXs[11] = { 110, 185, 260, 335, 410, 485, 560, 635, 710, 785, 830 };
        const char* knobLabels[11] = {
            "BIAS L", "BIAS R", "SAT L", "SAT R", "WOW/FLUT",
            "PRINT", "ST.ASYM", "MULTIPL", "MAINS HUM", "TAPE FRM", "HUM FRQ"
        };
        // Tape-alignment knob labels — engraved on dark service strip
        // UX: bumpad från 6.0 → 9.5 + rect-höjd 8 → 12 så texten faktiskt
        // går att läsa.  Tidigare 6px var oavläsbar mot mörk panel.
        {
            const auto knobFont = juce::Font (juce::FontOptions (9.5f, juce::Font::bold))
                                       .withExtraKerningFactor (0.08f);
            for (int i = 0; i < 11; ++i)
                drawEngraved (knobLabels[i],
                               juce::Rectangle<int> (slotXs[i] - 40, 429, 80, 12),
                               silk, knobFont);
        }

        // ===== Main control panel (black anodised) =====
        drawBlackPanel (60.0f, 442.0f, 800.0f, 296.0f, 2.0f);

        // ===== Right-zone sub-panels — recessed wells for PLUGIN UTILITY + ECHO =====
        // Each section gets its own framed look — slightly darker inset with melatonin
        // inner-shadow for depth + 1px top highlight for elevation.  This is the UAD/
        // Soundtoys signature: every functional group sits in its own subtly-recessed pad.
        auto drawSubPanel = [&] (juce::Rectangle<float> r, float corner)
        {
            // Inset darker fill (slightly darker than main panel)
            juce::ColourGradient sg (juce::Colour (0xFF0A0A0A), r.getX(), r.getY(),
                                       juce::Colour (0xFF1A1A1A), r.getX(), r.getBottom(), false);
            sg.addColour (0.5, juce::Colour (0xFF0E0E0E));
            g.setGradientFill (sg);
            g.fillRoundedRectangle (r, corner);
            // Melatonin inner shadow (cached)
            juce::Path pp;
            pp.addRoundedRectangle (r, corner);
            panelInnerShadow.render (g, pp);
            // Top edge highlight — bevel-into-cabinet feel
            g.setColour (juce::Colour (0x55FFFFFF));
            g.drawLine (r.getX() + 4, r.getY() + 0.5f, r.getRight() - 4, r.getY() + 0.5f, 0.7f);
            // Bottom edge shadow
            g.setColour (juce::Colour (0xAA000000));
            g.drawLine (r.getX() + 4, r.getBottom() - 0.5f, r.getRight() - 4, r.getBottom() - 0.5f, 0.5f);
            // Outline
            g.setColour (juce::Colour (0x99000000));
            g.drawRoundedRectangle (r, corner, 0.6f);
        };
        // PLUGIN UTILITY sub-panel — wraps INPUT/OUTPUT + BASS/TREBLE knob group
        drawSubPanel (juce::Rectangle<float> (594, 456, 220, 124), 4.0f);
        // ECHO sub-panel — wraps TIME/FEEDBACK knob group
        drawSubPanel (juce::Rectangle<float> (594, 595, 220, 96),  4.0f);

        // ===== Left-zone sub-panels — TRACK/ARM/OUTPUT/MONITOR groups =====
        // Same recessed look as the right zone → every functional group sits in its own pad.
        drawSubPanel (juce::Rectangle<float> ( 88, 456, 216, 46), 4.0f);  // TRACK / ARM / SYNC / PAUSE
        drawSubPanel (juce::Rectangle<float> ( 88, 504, 216, 42), 4.0f);  // OUTPUT MODE
        drawSubPanel (juce::Rectangle<float> ( 88, 580, 216, 50), 4.0f);  // MONITOR + S on S
        // OUTPUT MONITORING — speakers + headphones + VU-meters in one unified pad
        drawSubPanel (juce::Rectangle<float> ( 88, 632, 220, 80), 4.0f);

        // UAD-grade Phillips screws with recessed well + chrome head + cross-slot
        auto drawScrew = [&] (float cx, float cy)
        {
            // (1) Soft drop shadow
            for (int i = 2; i >= 1; --i)
            {
                const float a = 0.18f / (float) i;
                g.setColour (juce::Colour::fromFloatRGBA (0, 0, 0, a));
                g.fillEllipse (cx - 5.5f - (float) i * 0.3f, cy - 5.0f + (float) i * 0.3f,
                                (5.5f + (float) i * 0.3f) * 2.0f, (5.0f + (float) i * 0.3f) * 2.0f);
            }
            // (2) Recessed dark well around screw
            juce::ColourGradient wellGr (juce::Colour (0xFF030303), cx - 5, cy - 5,
                                          juce::Colour (0xFF1A1A1A), cx + 5, cy + 5, true);
            g.setGradientFill (wellGr);
            g.fillEllipse (cx - 5.5f, cy - 5.5f, 11.0f, 11.0f);
            // (3) Chrome screw head — radial gradient
            juce::ColourGradient sgr (juce::Colour (0xFFFFFFFF), cx - 1.5f, cy - 2.5f,
                                       juce::Colour (0xFF4A4A4A), cx + 2.5f, cy + 2.5f, true);
            sgr.addColour (0.3, juce::Colour (0xFFE0E0E0));
            sgr.addColour (0.7, juce::Colour (0xFF888888));
            g.setGradientFill (sgr);
            g.fillEllipse (cx - 4.5f, cy - 4.5f, 9.0f, 9.0f);
            // (4) Screw rim outline
            g.setColour (juce::Colour (0xFF202020));
            g.drawEllipse (cx - 4.5f, cy - 4.5f, 9.0f, 9.0f, 0.5f);
            // (5) Top specular highlight on chrome head
            g.setColour (juce::Colour (0xCCFFFFFF));
            g.fillEllipse (cx - 2.5f, cy - 3.0f, 3.0f, 1.4f);
            // (6) Phillips cross slot with depth (dark base + slight highlight on one edge)
            g.setColour (juce::Colour (0xFF050505));
            g.drawLine (cx - 3.0f, cy - 3.0f, cx + 3.0f, cy + 3.0f, 1.4f);
            g.drawLine (cx - 3.0f, cy + 3.0f, cx + 3.0f, cy - 3.0f, 1.4f);
            // (7) Slot inner brighter edge (slight directional reflection)
            g.setColour (juce::Colour (0x77FFFFFF));
            g.drawLine (cx - 2.5f, cy - 2.5f, cx + 0.0f, cy + 0.0f, 0.4f);
            g.setColour (juce::Colour (0x55FFFFFF));
            g.drawLine (cx + 0.0f, cy - 2.5f, cx + 2.5f, cy + 0.0f, 0.3f);
        };
        drawScrew (76,  458);
        drawScrew (844, 458);
        drawScrew (76,  722);
        drawScrew (844, 722);

        // ===== Left zone — engraved section headers + fontaudio glyph anchors =====
        // Small leading glyph + engraved header text — same pattern as preset bar / Service Strip.
        const auto drawSectionGlyph = [&] (const juce::String& glyph, int x, int y)
        {
            const auto glR = juce::Rectangle<float> ((float) x, (float) y, 10.0f, 10.0f);
            icons::drawIcon (g, glyph, glR.translated (0.4f, 0.4f),
                              juce::Colour (0xFF000000).withAlpha (0.55f), 9.0f);
            icons::drawIcon (g, glyph, glR, juce::Colour (LnF::kSilkDim), 9.0f);
        };
        drawEngraved ("TRACK / ARM / SYNC / PAUSE",
                       juce::Rectangle<int> (90, 456, 210, 12),
                       silkDim,
                       juce::Font (juce::FontOptions (9.0f, juce::Font::bold))
                            .withExtraKerningFactor (0.12f));
        drawSectionGlyph (icons::Record, 96, 457);

        // Track-row button labels — engraved
        {
            const auto trackFont = juce::Font (juce::FontOptions (8.5f, juce::Font::bold))
                                        .withExtraKerningFactor (0.05f);
            const char* leftRow1[6] = { "TRK 1", "TRK 2", "ARM L", "ARM R", "SYNC", "PAUSE" };
            const int leftRow1X[6]  = { 111, 141, 171, 201, 231, 261 };
            for (int i = 0; i < 6; ++i)
                drawEngraved (leftRow1[i],
                               juce::Rectangle<int> (leftRow1X[i] - 18, 484, 36, 11),
                               silk, trackFont);
        }

        // OUTPUT MODE — engraved header + Powerswitch glyph
        drawEngraved ("OUTPUT MODE",
                       juce::Rectangle<int> (100, 503, 180, 12),
                       silkDim,
                       juce::Font (juce::FontOptions (9.0f, juce::Font::bold))
                            .withExtraKerningFactor (0.18f));
        drawSectionGlyph (icons::Powerswitch, 96, 504);
        // Per-button labels — engraved
        {
            const auto modeFont = juce::Font (juce::FontOptions (8.5f, juce::Font::bold))
                                       .withExtraKerningFactor (0.08f);
            drawEngraved ("P.A.",     juce::Rectangle<int> (153, 528, 36, 11), silk, modeFont);
            drawEngraved ("AMP ONLY", juce::Rectangle<int> (197, 528, 42, 11), silk, modeFont);
        }

        // MONITOR + SoS — engraved section labels + glyphs
        {
            const auto sectionFont = juce::Font (juce::FontOptions (9.5f, juce::Font::bold))
                                          .withExtraKerningFactor (0.12f);
            // MONITOR header centered above combo (95-165 = center 130)
            drawEngraved ("MONITOR",
                           juce::Rectangle<int> (95, 583, 70, 13), silkDim, sectionFont);
            // S on S / MULTIPLAY header centered above button (180-202 = center 191)
            drawEngraved ("S on S / MULTIPLAY",
                           juce::Rectangle<int> (170, 583, 130, 13), silkDim, sectionFont);
        }
        drawSectionGlyph (icons::Headphones, 96, 584);
        drawSectionGlyph (icons::Loop,       176, 584);
        drawEngraved ("ON",
                       juce::Rectangle<int> (210, 610, 50, 11),  // centered under SoS button (235)
                       silk,
                       juce::Font (juce::FontOptions (8.5f, juce::Font::bold))
                            .withExtraKerningFactor (0.12f));

        // Speakers + Headphones — engraved section labels only.
        // (No fontaudio glyphs here: rocker buttons + jack circles already convey
        //  the section type — adding speaker/headphone glyphs collided with the
        //  Monitor-dropdown and SoS "ON" labels above, adding clutter.)
        {
            const auto sectionFont = juce::Font (juce::FontOptions (9.5f, juce::Font::bold))
                                          .withExtraKerningFactor (0.15f);
            drawEngraved ("SPEAKERS",
                           juce::Rectangle<int> (90, 634, 110, 12), silkDim, sectionFont);
            drawEngraved ("HEADPHONES",
                           juce::Rectangle<int> (210, 634, 90, 12), silkDim, sectionFont);
        }

        // ===== Drop shadow under each VU meter bezel (cached, melatonin) =====
        // Renders BEFORE VU Components paint themselves → bezel sits on a soft shadow halo.
        {
            const int meterX[4] = { 96, 149, 202, 255 };
            for (int x : meterX)
            {
                juce::Path vp;
                vp.addRoundedRectangle (juce::Rectangle<float> ((float) x, 675.0f, 50.0f, 22.0f), 2.5f);
                vuBezelShadow.render (g, vp);
            }
        }

        // ===== VU meter section labels — silkscreen "L"/"R" above each meter =====
        // Meters live at y=675 height=22, x=96/149/202/255, width=50 each
        {
            const int meterCenters[4] = { 121, 174, 227, 280 };  // x-centers of 4 meters
            const char* chLbl[4]      = { "L", "R", "L", "R" };
            // VU L/R channel labels — engraved
            {
                const auto chFont = juce::Font (juce::FontOptions (10.0f, juce::Font::bold))
                                         .withExtraKerningFactor (0.08f);
                for (int i = 0; i < 4; ++i)
                    drawEngraved (chLbl[i],
                                   juce::Rectangle<int> (meterCenters[i] - 10, 663, 20, 12),
                                   silk, chFont);
            }
            // INPUT / OUTPUT section labels — engraved
            {
                const auto secFont = juce::Font (juce::FontOptions (8.5f, juce::Font::bold))
                                          .withExtraKerningFactor (0.22f);
                drawEngraved ("INPUT",
                               juce::Rectangle<int> (96, 698, 103, 12), silkDim, secFont);
                drawEngraved ("OUTPUT",
                               juce::Rectangle<int> (202, 698, 103, 12), silkDim, secFont);
            }
        }
        // Hörtelefon jacks — chrome bezel + black socket
        // Centered under HEADPHONES header (header rect center x=255)
        for (int hx : { 240, 270 })
        {
            g.setColour (juce::Colour (LnF::kChromeMid));
            g.fillEllipse ((float) hx - 6, 652 - 6, 12, 12);
            g.setColour (juce::Colour (LnF::kStroke));
            g.drawEllipse ((float) hx - 6, 652 - 6, 12, 12, 0.8f);
            g.setColour (juce::Colour (0xFF101010));
            g.fillEllipse ((float) hx - 3, 652 - 3, 6, 6);
            g.setColour (juce::Colour (0x88FFFFFF));
            g.fillEllipse ((float) hx - 5, 652 - 5, 4, 2);
        }

        // ===== Fader zone — source-group labels (white silkscreen) + small fontaudio pictograms =====
        {
            const auto srcFont = juce::Font (juce::FontOptions (10.5f, juce::Font::bold))
                                      .withExtraKerningFactor (0.08f);
            struct SrcLabel { int x; const char* s; const juce::String* glyph; };
            // MASTER has no source-type — it's the overall level — so skip its glyph (null = no draw).
            const SrcLabel srcLabels[5] = {
                { 335, "RADIO",    &icons::Waveform   },
                { 390, "PHONO",    &icons::Diskio     },
                { 445, "MIC",      &icons::Microphone },
                { 500, "ECHO/SoS", &icons::Loop       },
                { 555, "MASTER",   nullptr            }
            };
            const auto iconBody   = juce::Colour (LnF::kSilk);                  // bright silk
            const auto iconShadow = juce::Colour (0xFF000000).withAlpha (0.55f);

            // Icon geometry: 11px glyph centered on label x, sitting tight above text.
            // Label text rect is y=461 h=10 (ends y=471). Icon box y=448 h=12 (ends y=460) → 1px gap.
            constexpr int kIconBoxW = 12;
            constexpr int kIconBoxH = 12;
            constexpr int kIconBoxY = 448;
            constexpr float kIconSize = 11.0f;

            for (auto& sl : srcLabels)
            {
                // Text label (centered on sl.x in an 80×10 box → text center == sl.x)
                drawEngraved (sl.s,
                               juce::Rectangle<int> (sl.x - 40, 461, 80, 10),
                               silk, srcFont);

                if (sl.glyph == nullptr)
                    continue;

                // Icon: 12×12 box centered on sl.x → icon glyph centered within box → centers match exactly
                const auto iconR = juce::Rectangle<int> (sl.x - kIconBoxW / 2, kIconBoxY,
                                                          kIconBoxW, kIconBoxH);
                icons::drawIcon (g, *sl.glyph,
                                  iconR.toFloat().translated (0.5f, 0.5f),
                                  iconShadow, kIconSize);
                icons::drawIcon (g, *sl.glyph, iconR, iconBody, kIconSize);
            }
        }

        // L/R sub-labels — engraved-style emboss above each fader
        {
            const auto lrFont = juce::Font (juce::FontOptions (8.5f, juce::Font::bold))
                                     .withExtraKerningFactor (0.08f);
            const int faderXs[10] = { 328, 342, 383, 397, 438, 452, 493, 507, 548, 562 };
            for (int i = 0; i < 10; ++i)
                drawEngraved ((i % 2 == 0) ? "L" : "R",
                               juce::Rectangle<int> (faderXs[i] - 7, 473, 14, 10),
                               silkDim, lrFont);
        }

        // ===== Fader scale (0-10) — UAD-grade engraved silkscreen with chrome ticks =====
        {
            const auto scaleFont = juce::Font (juce::FontOptions (9.0f, juce::Font::bold))
                                        .withExtraKerningFactor (0.05f);
            for (int t = 0; t <= 10; ++t)
            {
                const float y = juce::jmap ((float) t, 0.0f, 10.0f, 660.0f, 480.0f);
                // Tick shadow (subtle dark groove below)
                g.setColour (juce::Colour (0x99000000));
                g.drawLine (322, y + 0.5f, 326, y + 0.5f, 0.4f);
                g.drawLine (568, y + 0.5f, 572, y + 0.5f, 0.4f);
                // Tick body — bright chrome catch
                g.setColour (juce::Colour (0xFFEEEEEE));
                g.drawLine (322, y, 326, y, 0.6f);
                g.drawLine (568, y, 572, y, 0.6f);
                // Engraved number on both sides
                drawEngraved (juce::String (t),
                               juce::Rectangle<int> (308, (int) y - 5, 14, 10),
                               silk, scaleFont, juce::Justification::centredRight);
                drawEngraved (juce::String (t),
                               juce::Rectangle<int> (572, (int) y - 5, 14, 10),
                               silk, scaleFont, juce::Justification::centredLeft);
            }
        }

        // ===== Right zone — all labels engraved (UAD-grade emboss) =====
        {
            const auto headerFont = juce::Font (juce::FontOptions (9.0f, juce::Font::bold))
                                         .withExtraKerningFactor (0.22f);
            // labelFont krympt 9.5 → 8.5: knob-raderna har bara 8 px lucka
            // mellan sig (INPUT/OUTPUT-bottom 514, BASS/TREBLE-top 522), så större
            // text kollidera med knob ovan/under.  8.5pt är gränsen som passar.
            const auto labelFont  = juce::Font (juce::FontOptions (8.5f, juce::Font::bold))
                                         .withExtraKerningFactor (0.08f);

            // PLUGIN UTILITY header — flyttad UTANFÖR sub-panel (ovan, y=444)
            // för att frigöra plats inuti panel för knobs + labels utan kollision.
            // Sub-panel @ y=456-580; header sitter i 14-px-luckan ovanför.
            drawEngraved ("PLUGIN UTILITY",
                           juce::Rectangle<int> (590, 444, 220, 12), silkDim, headerFont);
            // Section glyph — Powerswitch (plugin utility = global I/O + tone)
            {
                const auto r = juce::Rectangle<float> (612.0f, 445.0f, 10.0f, 10.0f);
                icons::drawIcon (g, icons::Powerswitch, r.translated (0.4f, 0.4f),
                                  juce::Colour (0xFF000000).withAlpha (0.55f), 9.0f);
                icons::drawIcon (g, icons::Powerswitch, r, juce::Colour (LnF::kSilkDim), 9.0f);
            }
            // Row 1 — 4 plugin-utility-labels i 18-px lucka mellan knob-rader.
            // Centrerade på knob-centers 620/676/732/788 (label-rect = center-26).
            drawEngraved ("INPUT",  juce::Rectangle<int> (594, 511, 52, 11), silk, labelFont);
            drawEngraved ("OUTPUT", juce::Rectangle<int> (650, 511, 52, 11), silk, labelFont);
            drawEngraved ("MIX",    juce::Rectangle<int> (706, 511, 52, 11), silk, labelFont);
            drawEngraved ("NOISE",  juce::Rectangle<int> (762, 511, 52, 11), silk, labelFont);
            // Row 2 — BASS/TREBLE labels (knob-bottom 570, ECHO-header 602 = 32 px lucka)
            drawEngraved ("BASS",    juce::Rectangle<int> (620, 573, 56, 11), silk, labelFont);
            drawEngraved ("BALANCE", juce::Rectangle<int> (676, 573, 56, 11), silk, labelFont);
            drawEngraved ("TREBLE",  juce::Rectangle<int> (732, 573, 56, 11), silk, labelFont);
            // ECHO section header — INSIDE the sub-panel (panel top y=595).
            // Header sits near the panel's inner top edge; toggle button placed
            // below the header (y=620), knobs below the toggle (y=645).
            drawEngraved ("ECHO",
                           juce::Rectangle<int> (590, 602, 220, 12), silkDim, headerFont);
            // Section glyph — Loop (echo = recirculating delay)
            {
                const auto r = juce::Rectangle<float> (672.0f, 601.0f, 10.0f, 10.0f);
                icons::drawIcon (g, icons::Loop, r.translated (0.4f, 0.4f),
                                  juce::Colour (0xFF000000).withAlpha (0.55f), 9.0f);
                icons::drawIcon (g, icons::Loop, r, juce::Colour (LnF::kSilkDim), 9.0f);
            }
            // Row 3 — under TIME/FB (FEEDBACK needs ~52 wide, was 40 → "FEEDBAC" got cut)
            drawEngraved ("TIME",     juce::Rectangle<int> (632, 670, 56, 10), silk, labelFont);
            drawEngraved ("FEEDBACK", juce::Rectangle<int> (712, 670, 56, 10), silk, labelFont);
        }
    }

    //==========================================================================
    //  timerCallback — drive reel + VU animation from DSP chain
    //==========================================================================
    void WireframeEditor::timerCallback()
    {
        auto& chain = audioProc.getChain();

        // Speed-LED update
        const int speedIdx = cbSpeed.getSelectedItemIndex();
        if (speedIdx != prevSpeedIdx)
        {
            prevSpeedIdx = speedIdx;
            reelDeck.setSpeed (speedIdx);
        }

        // Active when not pause/stop and master_volume > 0
        const bool playing = ! btnMoment.getToggleState()
                          && (faderMasterL.getValue() > 0.001);
        reelDeck.setActive (playing);

        // VU levels from chain — INPUT meters read pre-DSP levels,
        // OUTPUT meters read post-master-volume levels.
        vuInL .setLevel (chain.inputLevelL_dBFS.load());
        vuInR .setLevel (chain.inputLevelR_dBFS.load());
        vuOutL.setLevel (chain.meterLevelL_dBFS.load());
        vuOutR.setLevel (chain.meterLevelR_dBFS.load());

        // ===== Counter (#5 RÄKNEVERK) — read tape position, format as 4-digit =====
        // Real Beocord counter showed reel rotations (~2 per second @ 9.5 cm/s).
        // We approximate as elapsed seconds × 2 → "0024" after ~12 sec at 9.5 speed.
        const double secs = chain.tapePositionSeconds.load (std::memory_order_relaxed);
        const float speedMul = (prevSpeedIdx == 0 ? 0.5f : prevSpeedIdx == 1 ? 1.0f : 2.0f);
        const int counterVal = ((int) (secs * 2.0 * speedMul)) % 10000;
        const juce::String newText = juce::String::formatted ("%04d", counterVal);
        if (newText != counterText)
        {
            counterText = newText;
            // Repaint only the counter rect (efficient)
            repaint (juce::Rectangle<int> (348, 60, 64, 24));
        }
    }
} // namespace bc2000dl::ui
