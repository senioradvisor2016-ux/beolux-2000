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
        // NORMAL=0 (Fe₂O₃), HIGH=1 (CrO₂) — schema 9224002 "NORMAL HIGH" switch.
        // CrO₂ kräver ~50 % mer bias för optimal magnetisering + har högre coercivity
        // (tightare hysteres-knä → mindre 3rd harm, bättre HF-respons).
        void setBiasType (int t);
        // 0 = 1/4 track stereo (BC2000 standard), 1 = 1/2 track (full-width).
        // 1/2 track: bredare gap → lägre brus + bredare head-bump-Q.
        void setTrackWidth (int t);

        void process (juce::AudioBuffer<float>& buffer, int channel);

    private:
        double sampleRate { 48000.0 };
        TapeSpeed currentSpeed { TapeSpeed::Speed19 };
        TapeFormula formula   { TapeFormula::Agfa };
        float biasAmount      { 1.0f };
        float saturationDrive { 1.0f };
        float printThrough    { 0.0f };
        int   biasType        { 0 };   // 0=NORMAL, 1=HIGH (CrO₂)
        int   trackWidth      { 0 };   // 0=1/4 track, 1=1/2 track

        // HF-roll-off per hastighet
        juce::dsp::IIR::Filter<float> hfFilter;

        // Head-bump (peaking EQ)
        juce::dsp::IIR::Filter<float> bumpFilter;

        // v60.3 — Bias-typ presence-shelf (HIGH = CrO₂ är ljusare ovanför 4 kHz).
        // Behövs eftersom hfCorner ofta clamps över Nyquist → ingen audibel
        // HF-skillnad mellan NORMAL/HIGH utan separat shelf-filter.
        juce::dsp::IIR::Filter<float> biasShelfFilter;

        // ----- Oversampled J-A hysteres-block -----
        // 8× oversampling kring J-A + bias-injection (100 kHz sinus).
        // Resampling-FIR-filter samt LP @ 25 kHz för bias-rejection.
        std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
        JilesAtherton hysteresis;
        juce::dsp::IIR::Filter<float> biasReject;  // LP @ 25 kHz

        // Bias-oscillator: 2D-rotation (4 muls + 2 adds per sample) istället för
        // std::sin(phase) (~5–10 ns × 768k samples/s = significant CPU).
        // (cos_state, sin_state) roteras med (cos_step, sin_step) per sample.
        // Konstant unit-amplitud (om cos²+sin²=1 vid init) — ingen drift.
        double biasCosState { 1.0 }, biasSinState { 0.0 };
        double biasCosStep  { 1.0 }, biasSinStep  { 0.0 };
        static constexpr double kBiasFreq_Hz = 100000.0;
        static constexpr int    kOversampleFactor = 3;  // 2^3 = 8×

        // Brusgenerator — LCG Gaussian (30× snabbare än mt19937)
        std::uint32_t lcgState { 0u };
        float noiseAmpLin { 0.0f };

        // DC-block för J-A remanens (en-pol HP, ~10 Hz)
        float dcBlockX1 { 0.0f }, dcBlockY1 { 0.0f };

        // Tyst-ingångsdetektor: om n_sil block i rad är under tröskeln
        // nollställs J-A-state för att eliminera M_r-remanens.
        // Konstant bias demagnetiserar inte i prakktiken (krävs avtagande
        // amplitud), så vi simulerar erasure-effekten digitalt.
        int silenceBlockCount { 0 };
        static constexpr int   kSilenceHoldBlocks = 1;
        static constexpr float kSilenceThreshPow  = 1e-9f; // ~−90 dBFS/sample

        // Print-through delay-buffer (~1.5 s)
        std::vector<float> printBuffer;
        int printIdx { 0 };

        void updateFilters();
    };
}
