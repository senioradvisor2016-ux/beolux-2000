/*  Ge2N2613Stage implementation — se header. */

#include "Ge2N2613Stage.h"
#include <cmath>
#include <algorithm>

namespace bc2000dl::dsp
{
    void Ge2N2613Stage::prepare (double sr,
                                 double gainDb,
                                 double channelAsym,
                                 std::uint32_t noiseSeed)
    {
        sampleRate  = sr;
        gainLinear  = std::pow (10.0, gainDb / 20.0);
        asymmetry   = kAsymmetryPNP + channelAsym;

        // Brus-skalning: vrms över 20 Hz–20 kHz, justerad till sample-rate
        constexpr double bandwidthAudio = 20000.0 - 20.0;
        const double bandwidthNyquist  = sr / 2.0;
        noiseSigma = kNoiseVrms_2N2613 * std::sqrt (bandwidthNyquist / bandwidthAudio);

        lcgState = noiseSeed != 0 ? noiseSeed
                                  : static_cast<std::uint32_t> (std::random_device {} ());
    }

    void Ge2N2613Stage::reset()
    {
        // Ingen filter-state att nollställa
    }

    void Ge2N2613Stage::setGain (double gainDb)
    {
        gainLinear = std::pow (10.0, gainDb / 20.0);
    }

    void Ge2N2613Stage::setChannelAsymmetry (double offset)
    {
        asymmetry = kAsymmetryPNP + offset;
    }

    double Ge2N2613Stage::softClip (double x, double asym, double Vt)
    {
        // Identisk matematik som Python ge_stages.py::_ebers_moll_softclip
        constexpr double scale = 0.9;
        const double xs = x * scale;

        // Asymmetri: olika knee per halvvåg → 2:a-harmonik-dominans
        const double baseKnee = std::max (Vt * 35.0, 0.5);
        const double a = std::clamp (asym * kAsymmetryGain, -0.7, 0.7);
        const double kneePos = baseKnee * (1.0 + a);
        const double kneeNeg = baseKnee * (1.0 - a);

        const double xPos = std::max (xs, 0.0);
        const double xNeg = std::max (-xs, 0.0);

        const double yPos = kneePos * std::tanh (xPos / kneePos);
        const double yNeg = -kneeNeg * std::tanh (xNeg / kneeNeg);

        return (yPos + yNeg) / scale;
    }

    float Ge2N2613Stage::processSample (float x)
    {
        // 1. Lägg till input-refererat brus
        const float noise = bc2000dl::dsp::detail::fastGaussNoise (lcgState)
                            * static_cast<float> (noiseSigma);
        const double xNoisy = static_cast<double> (x) + noise;

        // 2. Soft-clip med PNP-asymmetri (efter gain)
        const double clipped = softClip (xNoisy * gainLinear, asymmetry, kVT_25C);

        return static_cast<float> (clipped);
    }

    void Ge2N2613Stage::process (juce::AudioBuffer<float>& buffer, int channel)
    {
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
            data[i] = processSample (data[i]);
    }
}
