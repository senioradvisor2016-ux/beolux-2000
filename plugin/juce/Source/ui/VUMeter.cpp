/*  VUMeter — BC1500/2000 DE LUXE-stil VU.

    Kvadratisk format med:
      - Mörk navy/svart background
      - Vit graverad skala 20/10/7/5/3/2/1 + röd 0/1/2/3
      - Procent-skala underst (30/50/60/80/100 %)
      - B&O-logo centrerat i botten av skalan
      - Tunn svart visare från pivot vid botten
      - Silver-mesh-grille-zon underst
      - Liten chrome justerskruv i mitten av grille
*/

#include "VUMeter.h"
#include "AssetRegistry.h"

namespace bc2000dl::ui
{
    namespace
    {
        juce::Image& vuFaceImage() { return AssetRegistry::image (Asset::VuFace); }
    }

    using namespace bc2000dl::ui::colours;

    VUMeter::VUMeter (const juce::String& l)
        : label (l)
    {
        startTimerHz (30);
    }

    void VUMeter::pushLevel (float dbFs) { targetLevel.store (dbFs); }

    void VUMeter::timerCallback()
    {
        const float target = targetLevel.load();
        currentLevel = currentLevel * (1.0f - kSmoothCoef) + target * kSmoothCoef;
        repaint();
    }

    void VUMeter::paint (juce::Graphics& g)
    {
        // === TEAC-style VU meter (cream face, amber backlight) ===
        const auto outer = getLocalBounds().toFloat().reduced (1.0f);

        // 1. Yttre alu-bezel runt meter-fönstret
        juce::ColourGradient bezel (
            juce::Colour (0xffd4d1c8), outer.getX(), outer.getY(),
            juce::Colour (0xff8a8780), outer.getX(), outer.getBottom(), false);
        g.setGradientFill (bezel);
        g.fillRoundedRectangle (outer, 4.0f);
        g.setColour (juce::Colour (0xff141210));
        g.drawRoundedRectangle (outer, 4.0f, 0.7f);

        // Inner meter-area (hela utrymmet, ingen grille-zon)
        const auto scaleArea = outer.reduced (5.0f);
        const auto grilleArea = outer.reduced (5.0f);  // dummy för senare kod
        juce::ignoreUnused (grilleArea);

        // ----- TEAC cream-face med amber backlight glow -----
        // Bas-cream
        juce::ColourGradient cream (
            juce::Colour (0xfff0e8d0), scaleArea.getX(), scaleArea.getY(),
            juce::Colour (0xffd8c8a4), scaleArea.getX(), scaleArea.getBottom(), false);
        g.setGradientFill (cream);
        g.fillRoundedRectangle (scaleArea, 2.0f);

        // Noisehead-style djup amber-backlight (UAD VU-On/Off-pattern: dim när inactive)
        const float glowAlpha = active ? 0.78f : 0.22f;  // mer mättat än v15
        juce::ColourGradient amber (
            juce::Colour (0xffffb060).withAlpha (glowAlpha),  // varmare orange
            scaleArea.getCentreX(), scaleArea.getCentreY() + scaleArea.getHeight() * 0.20f,
            juce::Colour (0xffffb060).withAlpha (0.0f),
            scaleArea.getCentreX(),
            scaleArea.getCentreY() - scaleArea.getHeight() * 0.7f,
            true);
        g.setGradientFill (amber);
        g.fillRoundedRectangle (scaleArea, 2.0f);

        // Extra inner glow ring (höjer kontrast till glas-effekt)
        if (active)
        {
            juce::ColourGradient innerGlow (
                juce::Colour (0xffffd896).withAlpha (0.30f),
                scaleArea.getCentreX(), scaleArea.getBottom() - 8,
                juce::Colour (0xffffd896).withAlpha (0.0f),
                scaleArea.getCentreX(), scaleArea.getCentreY(), true);
            g.setGradientFill (innerGlow);
            g.fillRoundedRectangle (scaleArea, 2.0f);
        }

        // Glas-reflektion högst upp
        juce::ColourGradient gloss (
            juce::Colours::white.withAlpha (0.30f), scaleArea.getX(), scaleArea.getY(),
            juce::Colours::white.withAlpha (0.0f), scaleArea.getX(), scaleArea.getY() + scaleArea.getHeight() * 0.4f, false);
        g.setGradientFill (gloss);
        g.fillRoundedRectangle (scaleArea, 2.0f);

        // Inner rim (mörk)
        g.setColour (juce::Colour (0xff3a3530));
        g.drawRoundedRectangle (scaleArea, 2.0f, 0.7f);

        // ----- Skala-rendering -----
        // Pivot precis under meter-fönstret (nålen håller sig INOM bounds)
        const float pivotX = scaleArea.getCentreX();
        const float pivotY = scaleArea.getBottom() - 4.0f;
        const float radius = scaleArea.getHeight() * 0.85f;

        auto angleAt = [] (float v)
        {
            // -20 dBFS → -50°, 0 → 0°, +3 → +50°
            const float clamped = juce::jlimit (-20.0f, 5.0f, v);
            return juce::degreesToRadians ((-50.0f + (clamped + 20.0f) * (100.0f / 25.0f)));
        };

        // dB-skala-labels (TEAC-stil): 20 10 7 5 3 0 +3 över skalan
        struct Tick { float v; const char* label; bool red; };
        const Tick majors[] = {
            { -20.0f, "20", false }, { -10.0f, "10", false }, { -7.0f, "7", false },
            { -5.0f,  "5",  false }, { -3.0f,  "3",  false },
            {  0.0f,  "0",  true  }, {  3.0f,  "3", true  }
        };

        // Skalans tick-radie INOM meter-fönstret (säkerställer att labels syns)
        const float tickOuter = radius * 0.95f;
        const float tickInner = tickOuter - 5.0f;
        const float labelR    = tickInner - 7.0f;

        // Major ticks
        for (const auto& t : majors)
        {
            const float a = angleAt (t.v);
            const float x1 = pivotX + std::sin (a) * tickInner;
            const float y1 = pivotY - std::cos (a) * tickInner;
            const float x2 = pivotX + std::sin (a) * tickOuter;
            const float y2 = pivotY - std::cos (a) * tickOuter;
            g.setColour (t.red ? juce::Colour (0xffc02810) : juce::Colour (0xff141210));
            g.drawLine (x1, y1, x2, y2, 1.4f);

            // Label INNANFÖR ticken (mot pivot)
            const float tx = pivotX + std::sin (a) * labelR;
            const float ty = pivotY - std::cos (a) * labelR;
            g.setFont (juce::Font (juce::FontOptions (7.0f).withName ("Helvetica").withStyle ("Bold")));
            g.drawText (t.label,
                        juce::Rectangle<float> (tx - 8, ty - 4, 16, 8),
                        juce::Justification::centred, false);
        }

        // Minor ticks (samma radie-band som major)
        const float minorVals[] = { -15.0f, -8.0f, -6.0f, -4.0f, -2.0f, -1.0f, 1.0f, 2.0f };
        const float minorInner = tickOuter - 3.0f;
        for (float v : minorVals)
        {
            const float a = angleAt (v);
            const float x1 = pivotX + std::sin (a) * minorInner;
            const float y1 = pivotY - std::cos (a) * minorInner;
            const float x2 = pivotX + std::sin (a) * tickOuter;
            const float y2 = pivotY - std::cos (a) * tickOuter;
            g.setColour (v >= 0 ? juce::Colour (0xffc02810) : juce::Colour (0xff141210));
            g.drawLine (x1, y1, x2, y2, 0.8f);
        }

        // ----- "VU" text — under skalan, ovanför nålens pivot -----
        g.setColour (juce::Colour (0xff8a3010));  // mörk-orange/brun
        g.setFont (juce::Font (juce::FontOptions (10.0f).withName ("Helvetica").withStyle ("Bold")));
        g.drawText ("VU",
                    juce::Rectangle<float> (scaleArea.getCentreX() - 12,
                                             scaleArea.getBottom() - 22, 24, 10),
                    juce::Justification::centred, false);

        // ----- Liten gul LED + 2 button-cutouts på botten -----
        const float ledR = 2.5f;
        const float ledX = scaleArea.getCentreX();
        const float ledY = scaleArea.getBottom() - 6.0f;
        // LED-glow
        juce::ColourGradient ledGrad (
            juce::Colour (0xfffce080), ledX - ledR * 0.3f, ledY - ledR * 0.3f,
            juce::Colour (0xff885810), ledX + ledR, ledY + ledR, true);
        g.setGradientFill (ledGrad);
        g.fillEllipse (ledX - ledR, ledY - ledR, ledR * 2, ledR * 2);
        g.setColour (juce::Colour (0xff141210));
        g.drawEllipse (ledX - ledR, ledY - ledR, ledR * 2, ledR * 2, 0.4f);

        // 2 grey button-cutouts (en på var sida om LED)
        for (float dx : { -10.0f, 10.0f })
        {
            const auto btn = juce::Rectangle<float> (ledX + dx - 4, ledY - 2.5f, 8, 5);
            g.setColour (juce::Colour (0xff6a6862));
            g.fillRoundedRectangle (btn, 1.0f);
            g.setColour (juce::Colour (0xff141210).withAlpha (0.6f));
            g.drawRoundedRectangle (btn, 1.0f, 0.3f);
        }

        // ----- Noisehead-style nål: tjockare, med skugga -----
        const float t = juce::jlimit (-20.0f, 5.0f, currentLevel);
        const float a = angleAt (t);
        const float tipX = pivotX + std::sin (a) * (radius - 4.0f);
        const float tipY = pivotY - std::cos (a) * (radius - 4.0f);
        // Skugga
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawLine (pivotX + 0.5f, pivotY + 0.5f, tipX + 0.5f, tipY + 0.5f, 2.0f);
        // Nål (tjockare)
        g.setColour (juce::Colour (0xff141210));
        g.drawLine (pivotX, pivotY, tipX, tipY, 1.8f);

        // Pivot-prick (chrome cap med svart center)
        g.setColour (juce::Colour (0xffe8e5db));
        g.fillEllipse (pivotX - 4.5f, pivotY - 4.5f, 9.0f, 9.0f);
        g.setColour (juce::Colour (0xff8a8780));
        g.drawEllipse (pivotX - 4.5f, pivotY - 4.5f, 9.0f, 9.0f, 0.5f);
        g.setColour (juce::Colour (0xff141210));
        g.fillEllipse (pivotX - 2.0f, pivotY - 2.0f, 4.0f, 4.0f);

        // (Grille + skruv ej längre — TEAC-design är bara cream-face hela vägen)

        // ----- L/R-label uppe vänster (subtil) -----
        g.setColour (juce::Colour (0xff141210).withAlpha (0.55f));
        g.setFont (juce::Font (juce::FontOptions (7.0f).withName ("Helvetica").withStyle ("Bold")));
        g.drawText (label,
                    juce::Rectangle<float> (scaleArea.getX() + 4, scaleArea.getY() + 2, 30, 9),
                    juce::Justification::centredLeft, false);

        // ----- Record-arming: röd LED-glow runt hela meter -----
        if (recording)
        {
            g.setColour (juce::Colour (0xffd8442a).withAlpha (0.35f));
            g.drawRoundedRectangle (outer, 4.0f, 2.5f);
        }
    }
}
