/*  PhotorealReel implementation. */

#include "PhotorealReel.h"
#include "AssetRegistry.h"

namespace bc2000dl::ui
{
    juce::Image PhotorealReel::tryLoadAsset (Variant variant)
    {
        return AssetRegistry::image (variant == Variant::LeftWithTape
            ? Asset::ReelLeft : Asset::ReelRight);
    }

    juce::Image PhotorealReel::renderHighQuality (Variant variant, int sizePx)
    {
        // Pre-rendera vid 2× resolution för crisp downscale
        const int superSize = sizePx * 2;
        juce::Image img (juce::Image::ARGB, superSize, superSize, true);
        juce::Graphics g (img);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);

        const float cx = superSize * 0.5f;
        const float cy = superSize * 0.5f;
        const float r  = superSize * 0.46f;

        // ----- Layer 1: drop shadow (soft) -----
        // Rita en mörk cirkel offsetad ner-höger som "skugga", sedan blur
        {
            juce::Image shadow (juce::Image::ARGB, superSize, superSize, true);
            juce::Graphics sg (shadow);
            sg.setColour (juce::Colours::black.withAlpha (0.55f));
            sg.fillEllipse (cx - r + 6, cy - r + 8, r * 2, r * 2);
            renderSoftShadow (shadow, 8.0f);
            g.drawImageAt (shadow, 0, 0);
        }

        // ----- Layer 2: bas-disc (transparent acryl med gradient) -----
        renderBaseLayer (g, cx, cy, r, variant);

        // ----- Layer 3: brun tape-coil-wedges (bara LeftWithTape) -----
        if (variant == Variant::LeftWithTape)
            renderTapeWedges (g, cx, cy, r);

        // ----- Layer 4: highlight reflection (övre vänstra kvadranten) -----
        {
            juce::ColourGradient hlGrad (
                juce::Colours::white.withAlpha (0.45f),
                cx - r * 0.55f, cy - r * 0.75f,
                juce::Colours::white.withAlpha (0.0f),
                cx + r * 0.30f, cy + r * 0.20f, false);
            g.setGradientFill (hlGrad);
            g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
        }

        // ----- Layer 5: hub + label centrerat -----
        renderHubAndLabel (g, cx, cy, r, variant);

        // Downscale till slutlig storlek (juce::Image rescaled() ger crisp result)
        return img.rescaled (sizePx, sizePx, juce::Graphics::highResamplingQuality);
    }

    void PhotorealReel::renderBaseLayer (juce::Graphics& g, float cx, float cy, float r,
                                          Variant variant)
    {
        // Acryl-disc med radial gradient — ljust center, mörkare kant
        juce::ColourGradient discGrad (
            juce::Colour (0xfff8f5ec).withAlpha (variant == Variant::LeftWithTape ? 0.92f : 0.96f),
            cx - r * 0.3f, cy - r * 0.5f,
            juce::Colour (0xff9a978e).withAlpha (variant == Variant::LeftWithTape ? 0.65f : 0.78f),
            cx + r * 0.7f, cy + r * 0.7f, true);
        g.setGradientFill (discGrad);
        g.fillEllipse (cx - r, cy - r, r * 2, r * 2);

        // Yttre rim-kant (mörk metallisk linje)
        g.setColour (juce::Colour (0xff3a3832));
        g.drawEllipse (cx - r + 1, cy - r + 1, r * 2 - 2, r * 2 - 2, 1.4f);

        // Subtile inner ridge runt rim (chrome-bezel-känsla)
        g.setColour (juce::Colours::white.withAlpha (0.30f));
        g.drawEllipse (cx - r + 4, cy - r + 4, r * 2 - 8, r * 2 - 8, 0.7f);
    }

    void PhotorealReel::renderTapeWedges (juce::Graphics& g, float cx, float cy, float r)
    {
        // 3 brown pie-wedges med tape-windings
        const float tapeOuter = r * 0.86f;
        const float tapeInner = r * 0.30f;
        const float wedgeSpan = juce::degreesToRadians (90.0f);
        const float spokeSpan = juce::degreesToRadians (30.0f);
        const float unitSpan  = wedgeSpan + spokeSpan;
        const float baseRotation = juce::degreesToRadians (-90.0f);  // start uppe

        // Render brown wedges
        for (int s = 0; s < 3; ++s)
        {
            const float startA = baseRotation + s * unitSpan + spokeSpan * 0.5f;
            const float endA   = startA + wedgeSpan;

            juce::Path wedge;
            wedge.addPieSegment (cx - tapeOuter, cy - tapeOuter,
                                  tapeOuter * 2, tapeOuter * 2,
                                  startA, endA, tapeInner / tapeOuter);

            // Multi-stop radial brown gradient (mörkare i mitten, ljusare ute)
            juce::ColourGradient brownGrad (
                juce::Colour (0xff6a3818), cx, cy,
                juce::Colour (0xff8e5a3c), cx + tapeOuter, cy, true);
            brownGrad.addColour (0.3, juce::Colour (0xff7a4628));
            brownGrad.addColour (0.7, juce::Colour (0xff8e5a3c));
            g.setGradientFill (brownGrad);
            g.fillPath (wedge);

            // Inner edge-shadow (ger 3D-känsla på wedge)
            g.setColour (juce::Colour (0xff2a1408).withAlpha (0.45f));
            g.strokePath (wedge, juce::PathStrokeType (1.0f));
        }

        // Tape-windings: koncentriska arcs clipade till brown-areorna
        juce::Path windingClip;
        for (int s = 0; s < 3; ++s)
        {
            const float startA = baseRotation + s * unitSpan + spokeSpan * 0.5f;
            const float endA   = startA + wedgeSpan;
            windingClip.addPieSegment (cx - tapeOuter, cy - tapeOuter,
                                        tapeOuter * 2, tapeOuter * 2,
                                        startA, endA, tapeInner / tapeOuter);
        }
        {
            juce::Graphics::ScopedSaveState scoped (g);
            g.reduceClipRegion (windingClip);

            // Många koncentriska tape-windings för "stack"-illusion
            for (float rr = tapeInner + 0.5f; rr < tapeOuter; rr += 0.7f)
            {
                const float t = (rr - tapeInner) / (tapeOuter - tapeInner);
                const float baseAlpha = 0.10f + 0.08f * std::sin (rr * 0.6f);
                g.setColour (juce::Colour (0xff2a1808).withAlpha (baseAlpha + t * 0.04f));
                g.drawEllipse (cx - rr, cy - rr, rr * 2, rr * 2, 0.5f);
            }

            // Subtila brun-tone-band varje 15:e ring för djup
            for (int b = 0; b < 8; ++b)
            {
                const float rr = tapeInner + (tapeOuter - tapeInner) * (b + 0.5f) / 8.0f;
                g.setColour (juce::Colour (0xff5a3018).withAlpha (0.20f));
                g.drawEllipse (cx - rr, cy - rr, rr * 2, rr * 2, 1.4f);
            }
        }
    }

    void PhotorealReel::renderHubAndLabel (juce::Graphics& g, float cx, float cy, float r,
                                            Variant variant)
    {
        const float labelR = r * 0.18f;

        if (variant == Variant::LeftWithTape)
        {
            // Vänster: röd label
            juce::ColourGradient labelGrad (
                juce::Colour (0xffe04830), cx - labelR * 0.3f, cy - labelR * 0.3f,
                juce::Colour (0xff7a1808), cx + labelR, cy + labelR, true);
            g.setGradientFill (labelGrad);
            g.fillEllipse (cx - labelR, cy - labelR, labelR * 2, labelR * 2);
        }
        else
        {
            // Höger: EMITAPE yellow + red center
            g.setColour (juce::Colour (0xffe8c020));
            g.fillEllipse (cx - labelR, cy - labelR, labelR * 2, labelR * 2);

            g.setColour (juce::Colour (0xff141414));
            g.setFont (juce::Font (juce::FontOptions (labelR * 0.55f).withName ("Helvetica").withStyle ("Bold")));
            g.drawText ("EM",
                        juce::Rectangle<float> (cx - labelR, cy - labelR + 2, labelR * 2, labelR * 0.7f),
                        juce::Justification::centred, false);
            g.drawText ("TAPE",
                        juce::Rectangle<float> (cx - labelR, cy + labelR - labelR * 0.75f, labelR * 2, labelR * 0.7f),
                        juce::Justification::centred, false);

            const float redR = labelR * 0.55f;
            g.setColour (juce::Colour (0xffc82020));
            g.fillEllipse (cx - redR, cy - redR, redR * 2, redR * 2);
        }
        g.setColour (juce::Colour (0xff1a1410).withAlpha (0.7f));
        g.drawEllipse (cx - labelR, cy - labelR, labelR * 2, labelR * 2, 0.7f);

        // Chrome hub med bolt-holes
        const float hubR = labelR * 0.50f;
        juce::ColourGradient hubGrad (
            juce::Colour (0xfff0eee5), cx - hubR * 0.4f, cy - hubR * 0.4f,
            juce::Colour (0xff4a4842), cx + hubR * 0.7f, cy + hubR * 0.7f, true);
        g.setGradientFill (hubGrad);
        g.fillEllipse (cx - hubR, cy - hubR, hubR * 2, hubR * 2);
        g.setColour (juce::Colour (0xff141210));
        g.drawEllipse (cx - hubR, cy - hubR, hubR * 2, hubR * 2, 0.7f);

        // 3 bolt-holes
        for (int b = 0; b < 3; ++b)
        {
            const float ba = juce::degreesToRadians (b * 120.0f - 90.0f);
            const float bx = cx + std::cos (ba) * hubR * 0.55f;
            const float by = cy + std::sin (ba) * hubR * 0.55f;
            g.setColour (juce::Colour (0xff080604));
            g.fillEllipse (bx - 1.8f, by - 1.8f, 3.6f, 3.6f);
        }

        // Center spindel
        const float spR = hubR * 0.20f;
        g.setColour (juce::Colour (0xff080604));
        g.fillEllipse (cx - spR, cy - spR, spR * 2, spR * 2);
    }

    void PhotorealReel::renderSoftShadow (juce::Image& img, float radiusPx)
    {
        // Enkel box-blur för soft shadow (3-pass approximation av Gaussian)
        const int passes = 3;
        const int blurR  = (int) radiusPx;
        if (blurR <= 0) return;

        const int w = img.getWidth();
        const int h = img.getHeight();
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::readWrite);

        std::vector<juce::uint8> tmp (bd.size);

        for (int p = 0; p < passes; ++p)
        {
            // Horizontal pass
            std::memcpy (tmp.data(), bd.data, bd.size);
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    int sumA = 0, count = 0;
                    for (int dx = -blurR; dx <= blurR; ++dx)
                    {
                        const int sx = juce::jlimit (0, w - 1, x + dx);
                        sumA += tmp[(y * w + sx) * 4 + 3];
                        ++count;
                    }
                    bd.data[(y * w + x) * 4 + 3] = (juce::uint8) (sumA / count);
                }
            }
            // Vertical pass
            std::memcpy (tmp.data(), bd.data, bd.size);
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    int sumA = 0, count = 0;
                    for (int dy = -blurR; dy <= blurR; ++dy)
                    {
                        const int sy = juce::jlimit (0, h - 1, y + dy);
                        sumA += tmp[(sy * w + x) * 4 + 3];
                        ++count;
                    }
                    bd.data[(y * w + x) * 4 + 3] = (juce::uint8) (sumA / count);
                }
            }
        }
    }

    void PhotorealReel::paintProceduralReel (juce::Graphics& g, juce::Rectangle<float> bounds,
                                              Variant variant)
    {
        // Direkt-rita-fallback (samma resultat som high-quality men utan caching)
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float r  = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 4.0f;

        // Skugga
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillEllipse (cx - r + 2, cy - r + 4, r * 2, r * 2);

        renderBaseLayer (g, cx, cy, r, variant);
        if (variant == Variant::LeftWithTape) renderTapeWedges (g, cx, cy, r);

        // Highlight
        juce::ColourGradient hlGrad (
            juce::Colours::white.withAlpha (0.45f), cx - r * 0.55f, cy - r * 0.75f,
            juce::Colours::white.withAlpha (0.0f), cx + r * 0.30f, cy + r * 0.20f, false);
        g.setGradientFill (hlGrad);
        g.fillEllipse (cx - r, cy - r, r * 2, r * 2);

        renderHubAndLabel (g, cx, cy, r, variant);
    }
}
