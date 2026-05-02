/*  EqDIN1962 implementation. Använder JUCE:s IIR-cookbook (RBJ-shelves). */

#include "EqDIN1962.h"

namespace bc2000dl::dsp
{
    void SwitchedShelfEq::prepare (double sr)
    {
        sampleRate = sr;
        lfFilter.reset();
        hfFilter.reset();
        setConfig (currentCfg);
    }

    void SwitchedShelfEq::reset()
    {
        lfFilter.reset();
        hfFilter.reset();
    }

    void SwitchedShelfEq::setConfig (const Config& cfg)
    {
        currentCfg = cfg;

        // LF-shelf (boost LF, neutral high)
        if (std::abs (cfg.lfBoostDb) > 0.01f)
        {
            const auto coef = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
                sampleRate, cfg.lfCornerHz, 0.707f,
                juce::Decibels::decibelsToGain (cfg.lfBoostDb));
            lfFilter.coefficients = coef;
        }
        else
        {
            // Pass-through
            lfFilter.coefficients = new juce::dsp::IIR::Coefficients<float> (
                1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        }

        // HF-shelf
        if (std::abs (cfg.hfGainDb) > 0.01f)
        {
            const auto coef = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
                sampleRate, cfg.hfCornerHz, 0.707f,
                juce::Decibels::decibelsToGain (cfg.hfGainDb));
            hfFilter.coefficients = coef;
        }
        else
        {
            hfFilter.coefficients = new juce::dsp::IIR::Coefficients<float> (
                1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        }
    }

    float SwitchedShelfEq::processSample (float x)
    {
        return hfFilter.processSample (lfFilter.processSample (x));
    }

    void SwitchedShelfEq::process (juce::AudioBuffer<float>& buffer, int channel)
    {
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
            data[i] = processSample (data[i]);
    }

    // ---------- PlaybackEqDIN1962 ----------
    void PlaybackEqDIN1962::setSpeed (TapeSpeed speed)
    {
        Config cfg;
        switch (speed)
        {
            // LF-shelf reducerade 12/14/16 → 5/6/7 dB — tidigare gav den +
            // head-bump en samlad +6.7 dB peak vid 50 Hz vilket fail:ade ±3 dB.
            case TapeSpeed::Speed19:
                cfg = { 50.0f, 5.0f, 12000.0f, -2.0f }; break;
            case TapeSpeed::Speed95:
                cfg = { 50.0f, 6.0f, 7500.0f, -3.0f }; break;
            case TapeSpeed::Speed475:
                cfg = { 50.0f, 7.0f, 4000.0f, -4.0f }; break;
        }
        setConfig (cfg);
    }

    // ---------- PreEmphasisDIN1962 ----------
    void PreEmphasisDIN1962::setSpeed (TapeSpeed speed)
    {
        Config cfg;
        switch (speed)
        {
            case TapeSpeed::Speed19:
                cfg = { 0.0f, 0.0f, 15000.0f, 6.0f }; break;
            case TapeSpeed::Speed95:
                cfg = { 0.0f, 0.0f, 9000.0f, 9.0f }; break;
            case TapeSpeed::Speed475:
                cfg = { 0.0f, 0.0f, 5000.0f, 12.0f }; break;
        }
        setConfig (cfg);
    }
}
