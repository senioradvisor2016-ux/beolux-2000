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
                sampleRate, 100.0f, 0.707f,
                juce::Decibels::decibelsToGain (bassDb));
    }

    void ToneControl::updateTreble()
    {
        if (std::abs (trebleDb) < 0.01f)
            trebleFilter.coefficients = new juce::dsp::IIR::Coefficients<float> (
                1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        else
            trebleFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
                sampleRate, 10000.0f, 0.707f,
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
    void BalanceMaster::setBalance (float b) { balance = juce::jlimit (-1.0f, 1.0f, b); }
    void BalanceMaster::setMaster  (float m) { master  = juce::jlimit ( 0.0f, 1.0f, m); }

    void BalanceMaster::processStereo (juce::AudioBuffer<float>& buffer)
    {
        if (buffer.getNumChannels() < 2) return;

        // Equal-power-pan
        const float angle = (balance + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        const float lGain = std::cos (angle);
        const float rGain = std::sin (angle);

        // Linear master fade — fader-position matchar dB-respons mer intuitivt.
        // (Tidigare master*master gjorde 0.75 = -5 dB; nu 0.75 = -2.5 dB.)
        const float masterLin = master;

        const int n = buffer.getNumSamples();
        buffer.applyGain (0, 0, n, lGain * masterLin);
        buffer.applyGain (1, 0, n, rGain * masterLin);
    }
}
