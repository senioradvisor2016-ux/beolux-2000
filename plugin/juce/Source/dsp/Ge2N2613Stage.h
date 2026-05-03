/*  Ge2N2613Stage — PNP germanium small-signal amplifier

    Förekommer ~10 ggr i hela BC2000DL-kedjan. Modellerar:
    - Vit gaussisk brus (input-refererat)
    - Asymmetrisk soft-clip-waveshaper (Ebers-Moll-fit)
    - Frekvensoberoende gain

    Portad från Python-prototyp (plugin/dsp_prototype/ge_stages.py).
    Identisk matematik så att DAW-output matchar Python-validering.

    Plats: plugin/juce/Source/dsp/Ge2N2613Stage.h
*/

#pragma once

#include <random>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Constants.h"

namespace bc2000dl::dsp
{
    class Ge2N2613Stage
    {
    public:
        Ge2N2613Stage() = default;

        /** Ange sample-rate, gain (dB), per-channel-asymmetri (±0.05–0.10
            för autentisk 1968-tolerans), och random-seed. */
        void prepare (double sampleRate,
                      double gainDb = 20.0,
                      double channelAsymmetry = 0.0,
                      std::uint32_t noiseSeed = 0);

        /** Nollställ filter-state (ingen state i denna klass — placeholder). */
        void reset();

        /** Processa en enskild sample. */
        float processSample (float x);

        /** Processa en buffer (in-place). */
        void process (juce::AudioBuffer<float>& buffer, int channel);

        /** Sätt gain dynamiskt (dB). */
        void setGain (double gainDb);

        /** Sätt asymmetri-offset (för stereo-asymmetri). */
        void setChannelAsymmetry (double offset);

    private:
        double sampleRate { 48000.0 };
        double gainLinear { 10.0 };
        double asymmetry  { kAsymmetryPNP };
        double noiseSigma { 0.0 };

        std::uint32_t lcgState { 0u };   // fast LCG state (replaces mt19937)

        /** Ebers-Moll-derived asymmetric soft-clip. */
        static double softClip (double x, double asymmetry, double Vt);
    };
}
