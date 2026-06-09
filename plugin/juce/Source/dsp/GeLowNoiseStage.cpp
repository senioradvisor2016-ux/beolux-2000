/*  GeLowNoiseStage implementation. */

#include "GeLowNoiseStage.h"
#include "Ge2N2613Stage.h"  // för softClip-funktionen
#include <cmath>
#include <algorithm>
#include <random>   // std::random_device

namespace bc2000dl::dsp
{
    void GeLowNoiseStage::prepare (double sr,
                                   GeStageType t,
                                   double gainDb,
                                   double channelAsym,
                                   std::uint32_t noiseSeed)
    {
        sampleRate = sr;
        gainLinear = std::pow (10.0, gainDb / 20.0);
        type       = t;

        if (t == GeStageType::UW0029)
        {
            Is_value   = kIs_UW0029;
            asymmetry  = kAsymmetryPNP + channelAsym;
            // Brus
            constexpr double bandwidthAudio = 20000.0 - 20.0;
            noiseSigma = kNoiseVrms_UW0029 * std::sqrt ((sr / 2.0) / bandwidthAudio);
        }
        else // AC126
        {
            Is_value   = kIs_AC126;
            asymmetry  = kAsymmetryNPN + channelAsym;
            constexpr double bandwidthAudio = 20000.0 - 20.0;
            noiseSigma = kNoiseVrms_AC126 * std::sqrt ((sr / 2.0) / bandwidthAudio);
        }

        lcgState = noiseSeed != 0 ? noiseSeed
                                  : static_cast<std::uint32_t> (std::random_device {} ());
    }

    void GeLowNoiseStage::reset()
    {
        shaper.reset();   // ADAA-historik (prevU)
    }

    void GeLowNoiseStage::setGain (double gainDb)
    {
        gainLinear = std::pow (10.0, gainDb / 20.0);
    }

    void GeLowNoiseStage::setChannelAsymmetry (double offset)
    {
        const double base = (type == GeStageType::UW0029) ? kAsymmetryPNP : kAsymmetryNPN;
        asymmetry = base + offset;
    }

    float GeLowNoiseStage::processSample (float x)
    {
        const float noise = bc2000dl::dsp::detail::fastGaussNoise (lcgState)
                            * static_cast<float> (noiseSigma);
        const double xNoisy = static_cast<double> (x) + noise;
        // Samma Ebers-Moll-fit som Ge2N2613Stage, evaluerad via ADAA1
        // (GeSoftClip.h) — antialiserad i bas-samplerate.
        const double clipped = shaper.process (xNoisy * gainLinear, asymmetry, kVT_25C);
        return static_cast<float> (clipped);
    }

    void GeLowNoiseStage::process (juce::AudioBuffer<float>& buffer, int channel)
    {
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
            data[i] = processSample (data[i]);
    }
}
