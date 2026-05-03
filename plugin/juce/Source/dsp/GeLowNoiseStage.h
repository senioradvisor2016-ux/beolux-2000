/*  GeLowNoiseStage — UW0029 (PNP) eller AC126 (NPN) lågbrus-stage.

    Återanvänder samma soft-clip-matematik som Ge2N2613Stage men med:
    - Lägre Is (mjukare knee)
    - Lägre brus (utvald lågbrustyp)
    - Eventuellt spegelvänd asymmetri (NPN)

    Plats: plugin/juce/Source/dsp/GeLowNoiseStage.h
*/

#pragma once

#include <cstdint>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Constants.h"

namespace bc2000dl::dsp
{
    enum class GeStageType
    {
        UW0029,   // PNP — för mic / phono / radio input
        AC126     // NPN — för record / playback amps
    };

    class GeLowNoiseStage
    {
    public:
        GeLowNoiseStage() = default;

        void prepare (double sampleRate,
                      GeStageType type,
                      double gainDb = 30.0,
                      double channelAsymmetry = 0.0,
                      std::uint32_t noiseSeed = 0);

        void reset();
        float processSample (float x);
        void process (juce::AudioBuffer<float>& buffer, int channel);

        void setGain (double gainDb);
        void setChannelAsymmetry (double offset);

        GeStageType getType() const { return type; }

    private:
        double sampleRate { 48000.0 };
        double gainLinear { 30.0 };
        double asymmetry  { 0.0 };
        double Is_value   { 0.0 };
        double noiseSigma { 0.0 };
        GeStageType type  { GeStageType::UW0029 };

        std::uint32_t lcgState { 0u };   // fast LCG state (replaces mt19937)
    };
}
