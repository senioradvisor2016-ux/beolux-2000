/*  HybridHeroPanel — Ferroflux 2000 · DL bakgrund.

    Implementerar designen från "Tape Deck Plugin.html":
      - Walnut cheeks (44 px var, vänster + höger)
      - Cream brushed-metal frontpanel
      - Brand-header med "Ferroflux  2000 · DL" + center-tagline + power/rec-lamps
      - Footer-ribbon
*/

#include "HybridHeroPanel.h"
#include "AssetRegistry.h"

namespace bc2000dl::ui
{
    using namespace bc2000dl::ui::colours;

    namespace
    {
        constexpr float kCheekWidth   = 44.0f;
        constexpr float kHeaderInsetY = 18.0f;
        constexpr float kHeaderTextY  = 22.0f;
        constexpr float kHeaderH      = 56.0f;
        constexpr float kFooterH      = 26.0f;

        void drawWalnut (juce::Graphics& g, juce::Rectangle<float> r,
                          bool leftRounded, bool rightRounded)
        {
            // Diagonal warm gradient
            juce::ColourGradient grad (
                kWalnutHi, r.getX(), r.getY(),
                kWalnut2,  r.getRight(), r.getBottom(), false);
            grad.addColour (0.4, kWalnut1);
            g.setGradientFill (grad);

            const float radius = 8.0f;
            juce::Path p;
            // JUCE addRoundedRectangle med per-corner radii har specifik signatur:
            // (x, y, w, h, cornerSizeX, cornerSizeY, curveTopLeft, curveTopRight,
            //  curveBottomLeft, curveBottomRight)
            p.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                                    radius, radius,
                                    leftRounded,  rightRounded,
                                    leftRounded,  rightRounded);
            g.fillPath (p);

            // Vertikal träfiber-textur
            g.setColour (kWalnutDark.withAlpha (0.18f));
            for (float x = r.getX(); x < r.getRight(); x += 2.5f)
                g.drawLine (x, r.getY(), x, r.getBottom(), 0.4f);

            // Sub-fiber (horisontella små)
            g.setColour (kWalnutHi.withAlpha (0.10f));
            for (float y = r.getY(); y < r.getBottom(); y += 11.0f)
                g.drawLine (r.getX(), y, r.getRight(), y, 0.3f);

            // Mörkare rim
            g.setColour (kWalnutDark);
            g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);

            // Mässingsstift (top + botten centrerade)
            auto drawStud = [&] (float cx, float cy)
            {
                const float sr = 7.0f;
                juce::ColourGradient sg (
                    kCream1, cx - sr * 0.3f, cy - sr * 0.3f,
                    kInk,    cx + sr,        cy + sr, true);
                sg.addColour (0.55, kAmberDim);
                g.setGradientFill (sg);
                g.fillEllipse (cx - sr, cy - sr, sr * 2, sr * 2);
                g.setColour (kInk.withAlpha (0.5f));
                g.drawEllipse (cx - sr, cy - sr, sr * 2, sr * 2, 0.5f);
            };
            drawStud (r.getCentreX(), r.getY() + 14.0f);
            drawStud (r.getCentreX(), r.getBottom() - 14.0f);
        }

        void drawCreamFace (juce::Graphics& g, juce::Rectangle<float> r)
        {
            // Beocord 2000 DL brushed silver/aluminium-panel.
            // Verklig BC2000 DL har tunn brushed-aluminum-front (icke-cream).
            const juce::Colour silver1 (0xffd0cdc4);   // ljus silver topp
            const juce::Colour silver2 (0xffb6b3aa);   // mid
            const juce::Colour silver3 (0xff9c998f);   // skuggad

            juce::ColourGradient grad (
                silver1, r.getX(), r.getY(),
                silver3, r.getX(), r.getBottom(), false);
            grad.addColour (0.55, silver2);
            g.setGradientFill (grad);
            g.fillRect (r);

            // Brushed-metal-grain — horisontella linjer med subtil variation
            for (int i = 0; i < (int) r.getHeight(); i += 2)
            {
                const float a = ((i * 13) % 7) / 7.0f;
                g.setColour (juce::Colours::black.withAlpha (0.06f + a * 0.05f));
                g.drawLine (r.getX(), r.getY() + i, r.getRight(), r.getY() + i, 0.4f);
            }

            // Inre skugga längs ovan/under
            g.setColour (juce::Colours::black.withAlpha (0.30f));
            g.drawLine (r.getX(), r.getBottom() - 1, r.getRight(), r.getBottom() - 1, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.4f));
            g.drawLine (r.getX(), r.getY() + 0.5f, r.getRight(), r.getY() + 0.5f, 1.0f);
        }

        void drawScrew (juce::Graphics& g, float cx, float cy, float angleDeg)
        {
            const float sr = 6.5f;
            juce::ColourGradient grad (
                kCream1.brighter (0.25f), cx - sr * 0.3f, cy - sr * 0.4f,
                kInk,                     cx + sr,        cy + sr, true);
            grad.addColour (0.45, kCreamShadow);
            g.setGradientFill (grad);
            g.fillEllipse (cx - sr, cy - sr, sr * 2, sr * 2);

            g.setColour (kInk.withAlpha (0.6f));
            g.drawEllipse (cx - sr, cy - sr, sr * 2, sr * 2, 0.5f);

            // Phillips kryss
            const float a = juce::degreesToRadians (angleDeg);
            const float ca = std::cos (a), sa = std::sin (a);
            const float arm = sr * 0.7f;
            g.setColour (kInk);
            // strecken roterade
            const float x1 = cx + ca * arm, y1 = cy + sa * arm;
            const float x2 = cx - ca * arm, y2 = cy - sa * arm;
            g.drawLine (x1, y1, x2, y2, 1.0f);
            const float x3 = cx + sa * arm, y3 = cy - ca * arm;
            const float x4 = cx - sa * arm, y4 = cy + ca * arm;
            g.drawLine (x3, y3, x4, y4, 1.0f);
        }

        void drawEtched (juce::Graphics& g, juce::Rectangle<float> area,
                          juce::String text, float fontSize, juce::Justification just,
                          juce::Colour col, bool bold = false, float letterSpacing = 0.0f)
        {
            auto opt = juce::FontOptions (fontSize).withName ("Helvetica");
            if (bold) opt = opt.withStyle ("Bold");
            juce::Font f { opt };
            // letter-spacing approximation via extra spacing — skip for now, JUCE Font has no direct API
            g.setFont (f);

            // Ljus shadow för "etched" känsla
            g.setColour (kCream1.brighter (0.2f).withAlpha (0.6f));
            g.drawText (text, area.translated (0, 1), just, false);
            g.setColour (col);
            g.drawText (text, area, just, false);
            juce::ignoreUnused (letterSpacing);
        }
    }

    HybridHeroPanel::HybridHeroPanel()
    {
        setInterceptsMouseClicks (false, true);
        startTimerHz (30);
    }

    void HybridHeroPanel::timerCallback()
    {
        if (reelsRotating)
        {
            reelAngle += 0.06f;
            if (reelAngle > juce::MathConstants<float>::twoPi)
                reelAngle -= juce::MathConstants<float>::twoPi;
            repaint();
        }
    }

    void HybridHeroPanel::ensureReelImagesPrepared (int sizePx)
    {
        // Pre-rendera reel-images endast om size har ändrats (dyrt → bara en gång)
        if (sizePx != cachedReelSizePx || ! reelImageLeft.isValid())
        {
            cachedReelSizePx = sizePx;
            // Försök ladda från BinaryData först (om assets/reel_*.png finns)
            reelImageLeft  = PhotorealReel::tryLoadAsset (PhotorealReel::Variant::LeftWithTape);
            reelImageRight = PhotorealReel::tryLoadAsset (PhotorealReel::Variant::RightEmpty);
            // Fallback: pre-rendera high-quality procedural (cachas)
            if (! reelImageLeft.isValid())
                reelImageLeft = PhotorealReel::renderHighQuality (
                    PhotorealReel::Variant::LeftWithTape, sizePx);
            if (! reelImageRight.isValid())
                reelImageRight = PhotorealReel::renderHighQuality (
                    PhotorealReel::Variant::RightEmpty, sizePx);
        }
    }

    void HybridHeroPanel::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();

        // Yttre rumsbakgrund
        g.fillAll (kRoomBg);

        // ----- Vänster walnut cheek -----
        const auto leftCheek = bounds.withWidth (kCheekWidth);
        drawWalnut (g, leftCheek, true, false);

        // ----- Höger walnut cheek -----
        const auto rightCheek = bounds.withTrimmedLeft (bounds.getWidth() - kCheekWidth);
        drawWalnut (g, rightCheek, false, true);

        // ----- Cream face (mellan cheeks) -----
        const auto face = bounds.reduced (0).withTrimmedLeft (kCheekWidth)
                                              .withTrimmedRight (kCheekWidth);
        drawCreamFace (g, face);

        // Hörnskruvar
        drawScrew (g, face.getX() + 12.0f, face.getY() + 12.0f, 30.0f);
        drawScrew (g, face.getRight() - 12.0f, face.getY() + 12.0f, 120.0f);
        drawScrew (g, face.getX() + 12.0f, face.getBottom() - 12.0f, 75.0f);
        drawScrew (g, face.getRight() - 12.0f, face.getBottom() - 12.0f, 20.0f);

        // ===== AUTENTISK BC2000 DL LAYOUT =====
        // Top 38 % av face = tape-deck med 2 stora reels + speed-mech + B&O-handle
        // Bottom 62 % = brushed silver kontrollpanel (rymmer alla kontroller)
        const float deckTopY    = face.getY() + (float) headerHeight + 4.0f;
        const float deckH       = (face.getHeight() - headerHeight - footerHeight) * 0.30f;
        const float deckBottom  = deckTopY + deckH;

        // Stora reels — fyller deck-zonen autentiskt
        const float reelR  = juce::jmin (deckH * 0.42f, face.getWidth() * 0.16f);
        const float reelY  = deckTopY + deckH * 0.50f;
        const float reelLX = face.getX() + face.getWidth() * 0.27f;
        const float reelRX = face.getX() + face.getWidth() * 0.73f;

        // ===== Brushed aluminum-ram runt deck-cuvetten (matchar BC2000 foto) =====
        const juce::Rectangle<float> deckBg (
            face.getX() + 8.0f, deckTopY,
            face.getWidth() - 16.0f, deckH);

        // Noisehead-style drop-shadow under deck-sektion
        for (int i = 0; i < 4; ++i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.18f - i * 0.04f));
            g.fillRoundedRectangle (deckBg.translated (0.0f, 2.0f + i * 1.5f), 5.0f);
        }

        // Behåll procedural alu-frame som basen (TEAC-blocket overlayas senare CENTRERAT)
        juce::ColourGradient frameGrad (
            juce::Colour (0xffe2dfd5), deckBg.getX(), deckBg.getY(),
            juce::Colour (0xff9c998e), deckBg.getX(), deckBg.getBottom(), false);
        frameGrad.addColour (0.5, juce::Colour (0xffc8c5bb));
        g.setGradientFill (frameGrad);
        g.fillRoundedRectangle (deckBg, 5.0f);

        // Noisehead-style header-slats (3 horisontella vent-linjer ovanför deck)
        const float slatY = deckBg.getY() + 4.0f;
        for (int s = 0; s < 3; ++s)
        {
            const auto slat = juce::Rectangle<float> (
                deckBg.getX() + 12, slatY + s * 3, deckBg.getWidth() - 24, 1.5f);
            g.setColour (juce::Colour (0xff5a5852).withAlpha (0.55f));
            g.fillRoundedRectangle (slat, 0.5f);
        }

        // Brushed-grain (alltid på alu-ramen)
        for (int i = 0; i < (int) deckBg.getHeight(); i += 2)
        {
            const float a = ((i * 17) % 11) / 11.0f;
            g.setColour (juce::Colours::black.withAlpha (0.05f + a * 0.04f));
            g.drawLine (deckBg.getX() + 2, deckBg.getY() + i,
                        deckBg.getRight() - 2, deckBg.getY() + i, 0.4f);
        }

        // Inner deck-cuvette (mörk insänkt yta där reels sitter)
        const auto cuvette = deckBg.reduced (10.0f, 12.0f);
        juce::ColourGradient cuGrad (
            juce::Colour (0xff282520), cuvette.getX(), cuvette.getY(),
            juce::Colour (0xff141210), cuvette.getX(), cuvette.getBottom(), false);
        g.setGradientFill (cuGrad);
        g.fillRoundedRectangle (cuvette, 4.0f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawLine (cuvette.getX(), cuvette.getY(), cuvette.getRight(), cuvette.getY(), 1.5f);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawLine (cuvette.getX(), cuvette.getY() + 1.5f, cuvette.getRight(), cuvette.getY() + 1.5f, 0.7f);

        // Yttre rim (mörk metalliska kant)
        g.setColour (juce::Colour (0xff5a5852));
        g.drawRoundedRectangle (deckBg, 5.0f, 1.0f);

        // (TEAC-block-overlay borttagen — såg konstig ut som klistrad miniatyr.
        //  Vi behåller den rena dark-cuvette + B&O-handle-looken från v18.)

        // Hörnskruvar på aluminum-ramen (inte cuvetten)
        const float scrR = 5.0f;
        for (auto p : { juce::Point<float> (deckBg.getX() + 8, deckBg.getY() + 8),
                        juce::Point<float> (deckBg.getRight() - 8, deckBg.getY() + 8),
                        juce::Point<float> (deckBg.getX() + 8, deckBg.getBottom() - 8),
                        juce::Point<float> (deckBg.getRight() - 8, deckBg.getBottom() - 8) })
        {
            juce::ColourGradient scg (
                juce::Colour (0xffe5e2d8), p.x - scrR * 0.4f, p.y - scrR * 0.4f,
                juce::Colour (0xff5a5852), p.x + scrR, p.y + scrR, true);
            g.setGradientFill (scg);
            g.fillEllipse (p.x - scrR, p.y - scrR, scrR * 2, scrR * 2);
            g.setColour (juce::Colour (0xff141210));
            g.drawLine (p.x - scrR * 0.6f, p.y, p.x + scrR * 0.6f, p.y, 0.7f);
        }

        // ============= IMAGE-CACHED REELS (UAD-stil) =============
        // Pre-rendered high-quality reel-images cachas (vid första paint),
        // sedan ritas de med rotation-transform varje frame. Ger photoreal
        // kvalitet utan per-frame paint-overhead.
        const int reelSizePx = (int) (reelR * 2.0f) + 8;
        const_cast<HybridHeroPanel*> (this)->ensureReelImagesPrepared (reelSizePx);

        for (int reelIdx = 0; reelIdx < 2; ++reelIdx)
        {
            const float cx = (reelIdx == 0) ? reelLX : reelRX;
            const bool isLeft = (reelIdx == 0);
            const float rotateAngle = reelAngle + (isLeft ? 0.0f : 0.7f);
            const auto& cachedImg = isLeft ? reelImageLeft : reelImageRight;

            if (cachedImg.isValid())
            {
                // Skala foto-PNG (typiskt 1024px) → reelR*2 diameter
                juce::Graphics::ScopedSaveState scoped (g);
                const float halfSize  = (float) cachedImg.getWidth() * 0.5f;
                const float targetD   = reelR * 2.0f;
                const float scale     = targetD / (float) cachedImg.getWidth();
                g.addTransform (juce::AffineTransform::translation (-halfSize, -halfSize)
                                  .scaled (scale)
                                  .rotated (rotateAngle)
                                  .translated (cx, reelY));
                g.drawImage (cachedImg, 0, 0, cachedImg.getWidth(), cachedImg.getHeight(),
                              0, 0, cachedImg.getWidth(), cachedImg.getHeight());
                continue;
            }

            // Fallback om cache saknas (procedural)
            const float spokeRotation = reelAngle + (isLeft ? 0.0f : 0.7f);
            const float discR     = reelR;
            const float tapeOuter = reelR * 0.86f;
            const float tapeInner = reelR * 0.30f;

            // ----- 1. Yttre transparent acryl-disc (hela disken) -----
            juce::ColourGradient discGrad (
                juce::Colour (0xfff8f5ec).withAlpha (0.85f),
                cx - discR * 0.3f, reelY - discR * 0.5f,
                juce::Colour (0xffb0ada4).withAlpha (0.60f),
                cx + discR * 0.7f, reelY + discR * 0.7f, true);
            g.setGradientFill (discGrad);
            g.fillEllipse (cx - discR, reelY - discR, discR * 2, discR * 2);

            // Yttre rim-kant
            g.setColour (juce::Colour (0xff5a5852));
            g.drawEllipse (cx - discR + 0.5f, reelY - discR + 0.5f,
                           discR * 2 - 1, discR * 2 - 1, 0.7f);

            // ----- 2. Vänster reel: 3 brown pie-wedges (visar tape genom acryl) -----
            if (isLeft)
            {
                // 3 brown wedges (90° vardera) separerade av 30° acryl-spokes
                const float wedgeSpan = juce::degreesToRadians (90.0f);
                const float spokeSpan = juce::degreesToRadians (30.0f);
                const float unitSpan  = wedgeSpan + spokeSpan;  // 120°

                for (int s = 0; s < 3; ++s)
                {
                    // Vridning så spokes hamnar mellan wedges
                    const float startA = spokeRotation + s * unitSpan + spokeSpan * 0.5f;
                    const float endA   = startA + wedgeSpan;

                    juce::Path wedge;
                    wedge.addPieSegment (
                        cx - tapeOuter, reelY - tapeOuter,
                        tapeOuter * 2, tapeOuter * 2,
                        startA, endA,
                        tapeInner / tapeOuter);

                    // Brun tape-coil-fyllning (radial gradient — mörkare i mitten)
                    juce::ColourGradient brownGrad (
                        juce::Colour (0xff7a4628), cx, reelY,
                        juce::Colour (0xff8e5a3c), cx + tapeOuter, reelY, true);
                    g.setGradientFill (brownGrad);
                    g.fillPath (wedge);

                    // Mörkare kant runt wedge (subtil)
                    g.setColour (juce::Colour (0xff3a2010).withAlpha (0.35f));
                    g.strokePath (wedge, juce::PathStrokeType (0.6f));
                }

                // Tape-windings (koncentriska ringar) — clipade till bara wedge-areor
                juce::Path windingClip;
                for (int s = 0; s < 3; ++s)
                {
                    const float startA = spokeRotation + s * unitSpan + spokeSpan * 0.5f;
                    const float endA   = startA + wedgeSpan;
                    windingClip.addPieSegment (
                        cx - tapeOuter, reelY - tapeOuter,
                        tapeOuter * 2, tapeOuter * 2,
                        startA, endA,
                        tapeInner / tapeOuter);
                }
                {
                    juce::Graphics::ScopedSaveState scoped (g);
                    g.reduceClipRegion (windingClip);
                    for (float r = tapeInner + 1.0f; r < tapeOuter; r += 1.0f)
                    {
                        const float alpha = 0.13f + 0.10f * std::sin (r * 0.55f);
                        g.setColour (juce::Colour (0xff2a1808).withAlpha (alpha));
                        g.drawEllipse (cx - r, reelY - r, r * 2, r * 2, 0.45f);
                    }
                }
            }

            // ----- 3. Specular highlight på hela disken (övre vänstra) -----
            juce::ColourGradient hl (
                juce::Colours::white.withAlpha (0.40f),
                cx - discR * 0.5f, reelY - discR * 0.7f,
                juce::Colours::white.withAlpha (0.0f),
                cx + discR * 0.3f, reelY + discR * 0.3f, false);
            g.setGradientFill (hl);
            g.fillEllipse (cx - discR, reelY - discR, discR * 2, discR * 2);

            // ----- 4. Center label (på över spokes) -----
            const float labelR = discR * 0.18f;
            if (isLeft)
            {
                // Vänster: enkel röd label
                juce::ColourGradient labelGrad (
                    juce::Colour (0xffd84028), cx - labelR * 0.3f, reelY - labelR * 0.3f,
                    juce::Colour (0xff7a1808), cx + labelR, reelY + labelR, true);
                g.setGradientFill (labelGrad);
                g.fillEllipse (cx - labelR, reelY - labelR, labelR * 2, labelR * 2);
            }
            else
            {
                // Höger: EMITAPE — yellow donut + red center (utan text-spam)
                g.setColour (juce::Colour (0xffe8c020));
                g.fillEllipse (cx - labelR, reelY - labelR, labelR * 2, labelR * 2);
                // EM/TAPE-text på yellow donut (bara 2 ord, läsbart)
                g.setColour (juce::Colour (0xff141414));
                g.setFont (juce::Font (juce::FontOptions (8.5f).withName ("Helvetica").withStyle ("Bold")));
                g.drawText ("EM",
                            juce::Rectangle<float> (cx - labelR, reelY - labelR + 1, labelR * 2, 9),
                            juce::Justification::centred, false);
                g.drawText ("TAPE",
                            juce::Rectangle<float> (cx - labelR, reelY + labelR - 10, labelR * 2, 9),
                            juce::Justification::centred, false);
                // Röd center
                const float redR = labelR * 0.55f;
                g.setColour (juce::Colour (0xffc82020));
                g.fillEllipse (cx - redR, reelY - redR, redR * 2, redR * 2);
            }
            g.setColour (juce::Colour (0xff1a1410).withAlpha (0.7f));
            g.drawEllipse (cx - labelR, reelY - labelR, labelR * 2, labelR * 2, 0.6f);

            // ----- 5. Chrome hub + 3 bolt-holes + spindel -----
            const float hubR = labelR * 0.50f;
            juce::ColourGradient hubGrad (
                juce::Colour (0xfff0eee5), cx - hubR * 0.4f, reelY - hubR * 0.4f,
                juce::Colour (0xff4a4842), cx + hubR * 0.7f, reelY + hubR * 0.7f, true);
            g.setGradientFill (hubGrad);
            g.fillEllipse (cx - hubR, reelY - hubR, hubR * 2, hubR * 2);
            g.setColour (juce::Colour (0xff141210));
            g.drawEllipse (cx - hubR, reelY - hubR, hubR * 2, hubR * 2, 0.5f);

            // 3 bolt-holes
            for (int b = 0; b < 3; ++b)
            {
                const float ba = juce::degreesToRadians (b * 120.0f - 90.0f);
                const float bx = cx + std::cos (ba) * hubR * 0.55f;
                const float by = reelY + std::sin (ba) * hubR * 0.55f;
                g.setColour (juce::Colour (0xff080604));
                g.fillEllipse (bx - 1.4f, by - 1.4f, 2.8f, 2.8f);
            }

            // Center spindel
            const float spR = hubR * 0.20f;
            g.setColour (juce::Colour (0xff080604));
            g.fillEllipse (cx - spR, reelY - spR, spR * 2, spR * 2);
        }

        // ----- 5. Tape-ribbon mellan reels + ner under handle -----
        const float tapeRibbonCenterX = face.getCentreX();
        g.setColour (juce::Colour (0xff5a3220).withAlpha (0.85f));
        const float tapeY1 = reelY - reelR * 0.7f;
        const float tapeY2 = deckBottom - 30.0f;
        juce::Path tape;
        tape.startNewSubPath (reelLX + reelR * 0.7f, tapeY1);
        tape.cubicTo (reelLX + reelR * 0.4f, tapeY1 + 30,
                       reelRX - reelR * 0.4f, tapeY1 + 30,
                       reelRX - reelR * 0.7f, tapeY1);
        // Ner till heads under (head-cover)
        tape.startNewSubPath (tapeRibbonCenterX - 30.0f, reelY + reelR * 0.55f);
        tape.lineTo (tapeRibbonCenterX - 30.0f, tapeY2);
        tape.startNewSubPath (tapeRibbonCenterX + 30.0f, reelY + reelR * 0.55f);
        tape.lineTo (tapeRibbonCenterX + 30.0f, tapeY2);
        g.strokePath (tape, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved));

        // ===== Center mechanism (mellan reels) =====
        const float centerX = face.getCentreX();

        // ----- Speed-knob CHROME med vertikal ridge-kolumn (matchar foto) -----
        // Original BC2000 har speed som liten chrome-knob med 0/I/II/III runtom
        const float speedKnobR = 18.0f;
        const float speedKnobY = deckTopY + deckH * 0.18f;
        // Skugga
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (centerX - speedKnobR + 1, speedKnobY - speedKnobR + 2,
                       speedKnobR * 2, speedKnobR * 2);
        juce::ColourGradient sk (
            juce::Colour (0xffeae7de), centerX - speedKnobR * 0.4f, speedKnobY - speedKnobR * 0.6f,
            juce::Colour (0xff5a5852), centerX + speedKnobR * 0.7f, speedKnobY + speedKnobR * 0.7f, true);
        sk.addColour (0.55, juce::Colour (0xffb0ada4));
        g.setGradientFill (sk);
        g.fillEllipse (centerX - speedKnobR, speedKnobY - speedKnobR,
                       speedKnobR * 2, speedKnobR * 2);
        g.setColour (juce::Colour (0xff141414));
        g.drawEllipse (centerX - speedKnobR, speedKnobY - speedKnobR,
                       speedKnobR * 2, speedKnobR * 2, 0.8f);
        // Vertikal pekar-räffla på knob (matchar BC2000-knob-stil)
        g.setColour (juce::Colour (0xff141414));
        g.fillRect (centerX - 0.7f, speedKnobY - speedKnobR + 2.0f, 1.4f, 4.0f);

        // 0/I/II/III markings runt knob (mörka graveringar på aluminum)
        g.setColour (juce::Colour (0xff141414));
        g.setFont (juce::Font (juce::FontOptions (8.5f).withName ("Helvetica").withStyle ("Bold")));
        for (int i = 0; i < 4; ++i)
        {
            const char* lbl = (i == 0 ? "0" : i == 1 ? "I" : i == 2 ? "II" : "III");
            const float a = juce::degreesToRadians (-110.0f + i * 73.0f);
            const float lx = centerX + std::sin (a) * (speedKnobR + 11.0f);
            const float ly = speedKnobY - std::cos (a) * (speedKnobR + 11.0f);
            g.drawText (lbl, juce::Rectangle<float> (lx - 10, ly - 5, 20, 11),
                        juce::Justification::centred, false);
        }

        // ----- Chrome ridge-kolumn UNDER speed-knob (ned mot handle) -----
        const float ridgeY1 = speedKnobY + speedKnobR + 6.0f;
        const float ridgeY2 = reelY + reelR * 0.30f;
        const float ridgeW  = 22.0f;
        const auto ridgeRect = juce::Rectangle<float> (
            centerX - ridgeW * 0.5f, ridgeY1, ridgeW, ridgeY2 - ridgeY1);
        juce::ColourGradient rg (
            juce::Colour (0xffd0cdc4), ridgeRect.getX(), ridgeRect.getY(),
            juce::Colour (0xff7a7872), ridgeRect.getX(), ridgeRect.getBottom(), false);
        g.setGradientFill (rg);
        g.fillRoundedRectangle (ridgeRect, 1.5f);
        g.setColour (juce::Colour (0xff141414));
        g.drawRoundedRectangle (ridgeRect, 1.5f, 0.5f);
        // Vertikala ridges
        for (int r = 0; r < 6; ++r)
        {
            const float rx = ridgeRect.getX() + 3.0f + r * 3.0f;
            g.setColour (juce::Colour (0xff141414).withAlpha (0.4f));
            g.fillRect (rx, ridgeRect.getY() + 2, 0.7f, ridgeRect.getHeight() - 4);
            g.setColour (juce::Colours::white.withAlpha (0.4f));
            g.fillRect (rx + 1.0f, ridgeRect.getY() + 2, 0.5f, ridgeRect.getHeight() - 4);
        }

        // ----- B&O lifting handle — STÖRRE triangulär (autentisk BC2000) -----
        const float handleW = 130.0f, handleH = 70.0f;
        const float handleX = centerX - handleW * 0.5f;
        const float handleY = reelY + reelR * 0.30f;
        juce::Path handle;
        // Mer stilren triangel-form med rundade kanter
        handle.startNewSubPath (handleX, handleY + handleH);
        handle.lineTo (handleX + handleW * 0.10f, handleY + handleH * 0.30f);
        handle.cubicTo (handleX + handleW * 0.30f, handleY,
                         handleX + handleW * 0.70f, handleY,
                         handleX + handleW * 0.90f, handleY + handleH * 0.30f);
        handle.lineTo (handleX + handleW, handleY + handleH);
        handle.closeSubPath();
        // Skugga
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillPath (handle, juce::AffineTransform::translation (1.5f, 2.0f));
        // Handle-fill (mörkgrå plast med subtil gradient)
        juce::ColourGradient hg (
            juce::Colour (0xff7a7872), handleX, handleY,
            juce::Colour (0xff3a3832), handleX, handleY + handleH, false);
        g.setGradientFill (hg);
        g.fillPath (handle);
        g.setColour (juce::Colour (0xff141210));
        g.strokePath (handle, juce::PathStrokeType (0.8f));

        // Subtil ljus highlight längs övre kant
        g.setColour (juce::Colours::white.withAlpha (0.20f));
        g.drawLine (handleX + handleW * 0.30f, handleY + 2,
                    handleX + handleW * 0.70f, handleY + 2, 1.0f);

        // Krona + "B&O" centrerat
        g.setColour (juce::Colour (0xffe8e5dc));
        g.setFont (juce::Font (juce::FontOptions (16.0f).withName ("Helvetica")));
        g.drawText (juce::CharPointer_UTF8 ("\xe2\x99\x9b"),
                    juce::Rectangle<float> (handleX, handleY + handleH * 0.32f, handleW, 18),
                    juce::Justification::centred, false);
        g.setFont (juce::Font (juce::FontOptions (10.0f).withName ("Helvetica").withStyle ("Bold")));
        g.drawText ("B&O",
                    juce::Rectangle<float> (handleX, handleY + handleH * 0.62f, handleW, 14),
                    juce::Justification::centred, false);

        // ===== "BEOCORD 2000 DE LUXE" engraved title under reels =====
        const float titleY = deckBottom + 6.0f;
        g.setColour (juce::Colour (0xff141414));
        g.setFont (juce::Font (juce::FontOptions (12.0f).withName ("Helvetica")
                                .withStyle ("Bold")));
        g.drawText ("BEOCORD 2000 DE LUXE",
                    juce::Rectangle<float> (face.getX(), titleY, face.getWidth(), 18.0f),
                    juce::Justification::centred, false);

        // ===== Bottom kontrollpanel — DARK CHARCOAL (matchar BC2000 foto) =====
        const float panelY = titleY + 24.0f;
        const float panelH = face.getBottom() - panelY - footerHeight - 8.0f;
        const juce::Rectangle<float> panelBg (
            face.getX() + 8.0f, panelY, face.getWidth() - 16.0f, panelH);
        juce::ColourGradient pg (
            juce::Colour (0xff5a5852), panelBg.getX(), panelBg.getY(),
            juce::Colour (0xff3a3832), panelBg.getX(), panelBg.getBottom(), false);
        pg.addColour (0.5, juce::Colour (0xff4a4842));
        g.setGradientFill (pg);
        g.fillRoundedRectangle (panelBg, 3.0f);

        // Subtil texture
        for (int i = 0; i < (int) panelBg.getHeight(); i += 3)
        {
            g.setColour (juce::Colours::black.withAlpha (0.04f));
            g.drawLine (panelBg.getX(), panelBg.getY() + i,
                        panelBg.getRight(), panelBg.getY() + i, 0.3f);
        }

        // Top-highlight (chrome edge)
        g.setColour (juce::Colours::white.withAlpha (0.20f));
        g.drawLine (panelBg.getX() + 2, panelBg.getY() + 0.5f,
                    panelBg.getRight() - 2, panelBg.getY() + 0.5f, 1.0f);

        g.setColour (juce::Colour (0xff141210));
        g.drawRoundedRectangle (panelBg, 3.0f, 0.8f);

        // ----- Brand-header -----
        const auto header = face.withHeight (kHeaderH).reduced (24.0f, 0.0f)
                                  .withTrimmedTop (kHeaderInsetY);

        // Vänster: "BC2000DL" — engraved B&O-typografi
        {
            auto opt = juce::FontOptions (20.0f).withName ("Helvetica").withStyle ("Bold");
            g.setFont (juce::Font (opt));
            g.setColour (juce::Colours::black);
            g.drawText ("BC2000DL", header.withWidth (180.0f), juce::Justification::topLeft, false);

            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.setFont (juce::Font (juce::FontOptions (10.0f).withName ("Helvetica").withStyle ("Bold")));
            g.drawText ("DANISH TAPE 2000",
                        header.withWidth (260.0f).withTrimmedLeft (130.0f).translated (0, 4),
                        juce::Justification::topLeft, false);
        }

        // Center-tagline
        {
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.setFont (juce::Font (juce::FontOptions (8.5f).withName ("Helvetica").withStyle ("Bold")));
            g.drawText ("3-HEAD TAPE RECORDER  ·  100 kHz BIAS  ·  DIN 1962",
                        header, juce::Justification::centredTop, false);
        }

        // Höger: PWR / REC / TYPE 4119
        {
            auto right = header.withTrimmedLeft (header.getWidth() - 220.0f);

            // TYPE 4119
            g.setColour (kCreamShadow);
            g.setFont (juce::Font (juce::FontOptions (9.0f).withName ("Helvetica").withStyle ("Bold")));
            g.drawText ("TYPE 4119", right.translated (0, 4),
                        juce::Justification::topRight, false);

            // PWR-lamp + label
            const float lampY = right.getY() + 4.0f;
            const float lampSize = 7.0f;
            float lampX = right.getRight() - 180.0f;

            // PWR lamp (alltid på)
            juce::ColourGradient pwrGrad (kAmberGlow, lampX, lampY,
                                          kAmber, lampX + lampSize * 1.5f, lampY + lampSize * 1.5f, true);
            g.setGradientFill (pwrGrad);
            g.fillEllipse (lampX, lampY, lampSize, lampSize);
            g.setColour (kAmber.withAlpha (0.4f));
            g.fillEllipse (lampX - 2, lampY - 2, lampSize + 4, lampSize + 4);

            g.setColour (kInkSoft);
            g.setFont (juce::Font (juce::FontOptions (9.0f).withName ("Helvetica").withStyle ("Bold")));
            g.drawText ("PWR", juce::Rectangle<float> (lampX + lampSize + 4, lampY - 2,
                                                       28.0f, 12.0f),
                        juce::Justification::centredLeft, false);

            // REC lamp (röd, av i default-läge)
            lampX += 60.0f;
            g.setColour (juce::Colour (0xff3a1810));
            g.fillEllipse (lampX, lampY, lampSize, lampSize);
            g.setColour (kInk.withAlpha (0.4f));
            g.drawEllipse (lampX, lampY, lampSize, lampSize, 0.5f);

            g.setColour (kInkSoft);
            g.drawText ("REC", juce::Rectangle<float> (lampX + lampSize + 4, lampY - 2,
                                                       28.0f, 12.0f),
                        juce::Justification::centredLeft, false);
        }

        // Header underline
        g.setColour (kFaceLine.withAlpha (0.55f));
        g.drawLine (face.getX() + 24.0f, face.getY() + kHeaderH - 4.0f,
                    face.getRight() - 24.0f, face.getY() + kHeaderH - 4.0f, 1.0f);

        // ----- Footer ribbon -----
        const auto footer = face.withTrimmedTop (face.getHeight() - kFooterH).reduced (24.0f, 4.0f);

        g.setColour (kFaceLine.withAlpha (0.55f));
        g.drawLine (footer.getX(), footer.getY() - 2.0f,
                    footer.getRight(), footer.getY() - 2.0f, 1.0f);

        g.setColour (kInkSoft.withAlpha (0.85f));
        g.setFont (juce::Font (juce::FontOptions (8.0f).withName ("Helvetica").withStyle ("Bold")));
        g.drawText ("FERROFLUX TAPE SYSTEMS  ·  COPENHAGEN",
                    footer, juce::Justification::centredLeft, false);
        g.drawText ("SAMPLE 48 kHz  ·  LATENCY 0.4 ms  ·  ENGAGED",
                    footer, juce::Justification::centred, false);
        g.drawText ("DESIGNED & ASSEMBLED IN DK  ·  2026",
                    footer, juce::Justification::centredRight, false);
    }
}
