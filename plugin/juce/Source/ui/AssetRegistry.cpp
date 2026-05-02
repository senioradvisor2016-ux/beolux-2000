/*  AssetRegistry implementation. */

#include "AssetRegistry.h"
#include "BinaryData.h"
#include <unordered_map>

namespace bc2000dl::ui
{
    namespace
    {
        // Mappa Asset-enum → BinaryData-resursnamn (matchar
        // juce_add_binary_data SOURCES i CMakeLists.txt).
        const char* resourceName (Asset id)
        {
            switch (id)
            {
                case Asset::KnobFilmstrip:        return "knob_filmstrip_png";
                case Asset::KnobFilmstripBig:     return "knob_filmstrip_big_png";
                case Asset::KnobFilmstripMedium:  return "knob_filmstrip_medium_png";
                case Asset::KnobFilmstripSmall:   return "knob_filmstrip_small_png";
                case Asset::KnobMetal:            return "knob_metal_png";
                case Asset::FaderThumb:       return "fader_thumb_png";   // genereras v13
                case Asset::FaderTrack:       return "fader_track_png";   // genereras v13
                case Asset::BtnRect:          return "btn_rect_1_png";
                case Asset::BtnAccent:        return "btn_color_5_png";
                case Asset::BtnColor1:        return "btn_color_1_png";
                case Asset::BtnColor2:        return "btn_color_2_png";
                case Asset::BtnColor3:        return "btn_color_3_png";
                case Asset::BtnColor4:        return "btn_color_4_png";
                case Asset::BtnColor5:        return "btn_color_5_png";
                case Asset::ReelLeft:         return "reel_left_png";
                case Asset::ReelRight:        return "reel_right_png";
                case Asset::VuFace:           return "vu_face_png";
                case Asset::VuDial:           return "vu_dial_png";
                case Asset::ChassisAlu:       return "chassis_alu_png";
                case Asset::PanelMain:        return "panel_main_png";
                case Asset::PanelDarkStrip:   return "panel_dark_strip_png";
                case Asset::TeacDeckBg:       return "teac_deck_bg_png";
                case Asset::JackSmall:        return "jack_small_png";
            }
            return nullptr;
        }

        juce::Image loadFromBinary (const char* name)
        {
            if (name == nullptr) return {};
            int sz = 0;
            const char* d = BinaryData::getNamedResource (name, sz);
            if (d == nullptr || sz <= 0) return {};
            return juce::ImageFileFormat::loadFrom (d, (size_t) sz);
        }

        // Cache per asset (lazy)
        std::unordered_map<int, juce::Image>& cache()
        {
            static std::unordered_map<int, juce::Image> c;
            return c;
        }
    }

    juce::Image& AssetRegistry::image (Asset id)
    {
        auto& c = cache();
        auto key = static_cast<int> (id);
        auto it = c.find (key);
        if (it != c.end()) return it->second;
        auto img = loadFromBinary (resourceName (id));
        return c.emplace (key, std::move (img)).first->second;
    }

    void AssetRegistry::preloadAll()
    {
        // JackSmall är sista enum-värdet
        for (int i = 0; i <= static_cast<int> (Asset::JackSmall); ++i)
            image (static_cast<Asset> (i));
    }
}
