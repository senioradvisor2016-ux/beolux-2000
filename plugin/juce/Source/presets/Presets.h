/*  BC2000DL — Preset definitions (data-driven, validated).

    Rules:
      - mic_gain / mic_gain_r  >= 0.5   (signal always flows)
      - master_volume          >= 0.75  (never silent)
      - saturation_drive / _r  >= 0.5   (never gated)
      - bypass_tape is NOT stored — always reset to false when a preset fires
      - wow_flutter            <= 2.0   (plugin parameter max)
*/

#pragma once

namespace bc2000dl
{
    struct PresetData
    {
        const char* name         = "FACTORY";
        const char* category     = "ALL";
        const char* tip          = "";

        // --- Input faders (signal source) ---
        float mic_gain           = 0.5f;   // validated >= 0.5
        float mic_gain_r         = 0.5f;
        float phono_gain         = 0.0f;
        float phono_gain_r       = 0.0f;
        float radio_gain         = 0.0f;
        float radio_gain_r       = 0.0f;

        // --- Tape machine character ---
        int   tape_formula       = 0;      // 0=Agfa, 1=BASF, 2=Scotch
        int   speed              = 2;      // 0=4.75, 1=9.5, 2=19 cm/s
        float saturation_drive   = 1.0f;   // validated >= 0.5
        float saturation_drive_r = 1.0f;
        float bias_amount        = 1.0f;
        float wow_flutter        = 0.3f;
        int   multiplay_gen      = 1;      // 1–5

        // --- Tone & level ---
        float bass_db            = 0.0f;
        float treble_db          = 0.0f;
        float balance            = 0.0f;
        float master_volume      = 0.85f;  // validated >= 0.75

        // --- Echo ---
        bool  echo_enabled       = false;
        float echo_amount        = 0.0f;
        float echo_amount_r      = 0.0f;
    };

    static constexpr int kNumPresets    = 43;
    static constexpr int kNumCategories = 8;

    /** Full preset list — index 0 is FACTORY (the "reset" state). */
    extern const PresetData kPresets[kNumPresets];

    /** Category names in display order (index 0 = "ALL" shows everything). */
    extern const char* const kCategories[kNumCategories];
}
