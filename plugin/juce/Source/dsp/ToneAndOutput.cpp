/*  ToneControl + BalanceMaster implementation. */

#include "ToneAndOutput.h"
#include <cmath>

namespace bc2000dl::dsp
{
    // ---------- ToneControl ----------
    void ToneControl::prepare (double sr)
    {
        sampleRate = sr;
        updateBass();
        updateTreble();
    }

    void ToneControl::reset()
    {
        bassFilter.reset();
        trebleFilter.reset();
    }

    void ToneControl::setBassDb (float db)
    {
        bassDb = juce::jlimit (-12.0f, 12.0f, db);
        updateBass();
    }

    void ToneControl::setTrebleDb (float db)
    {
        trebleDb = juce::jlimit (-12.0f, 12.0f, db);
        updateTreble();
    }

    void ToneControl::updateBass()
    {
        if (std::abs (bassDb) < 0.01f)
            bassFilter.coefficients = new juce::dsp::IIR::Coefficients<float> (
                1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        else
            bassFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
                sampleRate, 300.0f, 0.56f,
                juce::Decibels::decibelsToGain (bassDb));
    }

    void ToneControl::updateTreble()
    {
        if (std::abs (trebleDb) < 0.01f)
            trebleFilter.coefficients = new juce::dsp::IIR::Coefficients<float> (
                1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        else
            trebleFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
                sampleRate, 3500.0f, 0.56f,
                juce::Decibels::decibelsToGain (trebleDb));
    }

    float ToneControl::processSample (float x)
    {
        return trebleFilter.processSample (bassFilter.processSample (x));
    }

    void ToneControl::process (juce::AudioBuffer<float>& buffer, int channel)
    {
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
            data[i] = processSample (data[i]);
    }

    // ---------- BalanceMaster ----------
    void BalanceMaster::prepare (double sr, int /*maxBlock*/)
    {
        constexpr double kSmoothSecs = 0.010;   // 10 ms — removes zipper noise at all block sizes
        masterSmooth.reset  (sr, kSmoothSecs);
        balanceSmooth.reset (sr, kSmoothSecs);
        masterSmooth.setCurrentAndTargetValue  (master);
        balanceSmooth.setCurrentAndTargetValue (balance);
    }

    void BalanceMaster::reset()
    {
        masterSmooth.setCurrentAndTargetValue  (master);
        balanceSmooth.setCurrentAndTargetValue (balance);
    }

    void BalanceMaster::setBalance (float b)
    {
        balance = juce::jlimit (-1.0f, 1.0f, b);
        balanceSmooth.setTargetValue (balance);
    }

    void BalanceMaster::setMaster (float m)
    {
        master = juce::jlimit (0.0f, 1.0f, m);
        masterSmooth.setTargetValue (master);
    }

    void BalanceMaster::processStereo (juce::AudioBuffer<float>& buffer)
    {
        if (buffer.getNumChannels() < 2) return;

        const int n = buffer.getNumSamples();

        if (masterSmooth.isSmoothing() || balanceSmooth.isSmoothing())
        {
            // Per-sample path: interpolate both controls during automation transitions.
            // This prevents zipper noise on fades and balance sweeps.
            auto* l = buffer.getWritePointer (0);
            auto* r = buffer.getWritePointer (1);
            for (int i = 0; i < n; ++i)
            {
                const float m = masterSmooth.getNextValue();
                const float ang = (balanceSmooth.getNextValue() + 1.0f)
                                  * juce::MathConstants<float>::pi * 0.25f;
                l[i] *= std::cos (ang) * m;
                r[i] *= std::sin (ang) * m;
            }
        }
        else
        {
            // Fast vectorised path: steady state (no automation change in flight).
            masterSmooth.skip  (n);
            balanceSmooth.skip (n);

            const float angle   = (balance + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            buffer.applyGain (0, 0, n, std::cos (angle) * master);
            buffer.applyGain (1, 0, n, std::sin (angle) * master);
        }
    }
}
