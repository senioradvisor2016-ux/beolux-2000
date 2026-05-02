/*  PhotorealReel — high-quality image-cached reel-renderer.

    Strategi (matchar UAD/Soundtoys-arkitektur):
      1. Pre-rendera reel ONCE som juce::Image vid 2× oversample
      2. Multi-pass rendering med soft-shadow Gaussian blur
      3. Cache image; runtime ritar bara med rotation-transform
      4. Drop-in-fil: om plugin/juce/Source/assets/reel_left.png /
         reel_right.png finns, använd dessa istället för procedural

    Detta ger photoreal-look utan per-frame paint-overhead.
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

namespace bc2000dl::ui
{
    class PhotorealReel
    {
    public:
        // Renderer-typ: full reel med tape, eller tom reel
        enum class Variant { LeftWithTape, RightEmpty };

        // Pre-render högkvalitets-image (call ONCE i HybridHeroPanel-konstruktor)
        static juce::Image renderHighQuality (Variant variant, int sizePx);

        // Procedural fallback om asset-fil inte finns
        static void paintProceduralReel (juce::Graphics& g, juce::Rectangle<float> bounds,
                                          Variant variant);

        // Försök ladda asset från BinaryData (returnera ogiltig Image om saknas)
        static juce::Image tryLoadAsset (Variant variant);

    private:
        static void renderBaseLayer (juce::Graphics& g, float cx, float cy, float r,
                                     Variant variant);
        static void renderTapeWedges (juce::Graphics& g, float cx, float cy, float r);
        static void renderHubAndLabel (juce::Graphics& g, float cx, float cy, float r,
                                        Variant variant);
        static void renderSoftShadow (juce::Image& img, float radiusPx);
    };
}
