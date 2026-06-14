/*  GeLowNoiseStage — UW0029 (kisel-NPN) eller AC126 (germanium-PNP) lågbrus-stage.

    Delar soft-clip-matematik med Ge2N2613Stage men differentierar per typ:
    - UW0029 = KISEL-NPN (BC109-ekv, Studio Sound 1968): symmetriskt knä
      (kAsymmetrySi=0), bredare knä (eff. Vt ×kSiKneeFactor), lägre brus.
    - AC126  = germanium-PNP: mjukare/asymmetriskt knä (h2-värme), högre brus.

    Germanium-färgningen i kedjan kommer från 2N2613/AC126 nedströms — inte
    ingångssteget. Se specs.md §12 (✅ LÖST). Plats: GeLowNoiseStage.h
*/

#pragma once

#include <cstdint>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Constants.h"
#include "GeSoftClip.h"

namespace bc2000dl::dsp
{
    enum class GeStageType
    {
        UW0029,   // KISEL-NPN (BC109-ekv) — mic / phono / radio input
        AC126     // germanium PNP — record / playback amps (8004005/6)
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
        double kneeVt     { kVT_25C };   // effektiv Vt för knäbredd (kisel = ×kSiKneeFactor)
        double noiseSigma { 0.0 };
        GeStageType type  { GeStageType::UW0029 };

        std::uint32_t lcgState { 0u };   // fast LCG state (replaces mt19937)

        /** Ebers-Moll-fit soft-clip med ADAA1 — se GeSoftClip.h. */
        detail::GeSoftClipADAA shaper;
    };
}
