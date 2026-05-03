/*  TapeSaturation — kärnblocket.

    B-H-hysteres-approximation (anhysteretisk Langevin via tanh) + 100 kHz
    bias-linjarisering implicit i knee-skalningen + per-hastighet HF-roll-off
    + head-bump + tape-egenbrus.

    Plats: plugin/juce/Source/dsp/TapeSaturation.h
*/

#pragma once

#include <cstdint>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Constants.h"
#include "JilesAtherton.h"

namespace bc2000dl::dsp
{
    /** Tape-formel — påverkar bias-optimum, HF-respons, brus och harmonisk balans.
        Tre 1968-typformler från plan §7 + bandtypsförslag. */
    enum class TapeFormula
    {
        Agfa,    // 1968 default — varmt, mid-forward, 3rd-harmonic-tendens
        BASF,    // raffinerad LF/HF, bättre HF-headroom, mindre ackumulation
        Scotch   // amerikansk klassiker — pronouncerad 2nd harm., mer kompression
    };

    class TapeSaturation
    {
    public:
        TapeSaturation() = default;

        void prepare (double sampleRate, std::uint32_t noiseSeed = 0);
        void reset();
        void setSpeed (TapeSpeed speed);
        void setBiasAmount (float biasAmount);     // 0.5–1.5 (1.0 = nominell)
        void setSaturationDrive (float drive);     // 0.5–2.0
        void setPrintThrough (float amount);       // 0.0–0.05
        void setFormula (TapeFormula f);

        void process (juce::AudioBuffer<float>& buffer, int channel);

    private:
        double sampleRate { 48000.0 };
        TapeSpeed currentSpeed { TapeSpeed::Speed19 };
        TapeFormula formula   { TapeFormula::Agfa };
        float biasAmount      { 1.0f };
        float saturationDrive { 1.0f };
        float printThrough    { 0.0f };

        // HF-roll-off per hastighet
        juce::dsp::IIR::Filter<float> hfFilter;

        // Head-bump (peaking EQ)
        juce::dsp::IIR::Filter<float> bumpFilter;

        // ----- Oversampled J-A hysteres-block -----
        // 8× oversampling kring J-A + bias-injection (100 kHz sinus).
        // Resampling-FIR-filter samt LP @ 25 kHz för bias-rejection.
        std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
        JilesAtherton hysteresis;
        juce::dsp::IIR::Filter<float> biasReject;  // LP @ 25 kHz
        double biasPhase { 0.0 };
        static constexpr double kBiasFreq_Hz = 100000.0;
        static constexpr int    kOversampleFactor = 3;  // 2^3 = 8×

        // Brusgenerator — LCG Gaussian (30× snabbare än mt19937)
        std::uint32_t lcgState { 0u };
        float noiseAmpLin { 0.0f };

        // Print-through delay-buffer (~1.5 s)
        std::vector<float> printBuffer;
        int printIdx { 0 };

        void updateFilters();
    };
}
