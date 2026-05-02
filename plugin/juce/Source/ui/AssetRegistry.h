/*  AssetRegistry — centraliserad ID→Image-lookup.

    Mönster från UAD Console (Houston/Rack), `StringIDToImagePath`.
    Mappar logiska asset-ID till BinaryData-resurser. Eliminerar
    hårdkodade resource-namn spridda över olika UI-klasser.

    Användning:
        auto& img = AssetRegistry::image (Asset::KnobFilmstrip);
        if (img.isValid()) g.drawImage (img, ...);

    Lägger till nya assets:
        1. CMakeLists.txt → juce_add_binary_data
        2. AssetRegistry.h → ny enum-värd
        3. AssetRegistry.cpp → mapping i kAssetMap
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bc2000dl::ui
{
    enum class Asset
    {
        // Knobs
        KnobFilmstrip,         // Default 60-frame UAD-style rotation strip
        KnobFilmstripBig,      // Noisehead organic-cap (för stora knobs)
        KnobFilmstripMedium,   // Noisehead translucent (medium)
        KnobFilmstripSmall,    // Noisehead chrome (små)
        KnobMetal,             // Single-frame fallback

        // Faders
        FaderThumb,         // Procedural fallback
        FaderTrack,         // Procedural fallback

        // Buttons
        BtnRect,            // BC2000 rectangular gray button
        BtnAccent,          // Active/toggled (red)
        BtnColor1,          // Cream colored
        BtnColor2,
        BtnColor3,          // Green
        BtnColor4,
        BtnColor5,          // Red

        // Reels
        ReelLeft,           // Full reel with brown tape
        ReelRight,          // Empty reel

        // VU Meters
        VuFace,             // Cream-amber TEAC face
        VuDial,             // Wider dial reference

        // Chassis
        ChassisAlu,         // Brushed aluminum top band
        PanelMain,          // Dark control-panel backdrop
        PanelDarkStrip,     // Selector-strip background
        TeacDeckBg,         // Photo TEAC deck-zone (capstan + chassis bakgrund)

        // Misc
        JackSmall,          // Phones jack
    };

    class AssetRegistry
    {
    public:
        // Returnera cachad juce::Image (laddas en gång, sen cached)
        static juce::Image& image (Asset id);

        // Pre-load alla assets (anropa från PluginEditor-konstruktor)
        static void preloadAll();

    private:
        AssetRegistry() = delete;
    };
}
