/*  PhonoPreamp implementation. */

#include "PhonoPreamp.h"

namespace bc2000dl::dsp
{
    void PhonoPreamp::prepare (double sr, float asym, std::uint32_t baseSeed)
    {
        sampleRate = sr;
        // H-läge: hög gain (~50 dB total)
        // L-läge: lägre gain (~25 dB total)
        const double gainUw = (mode == PhonoMode::H) ? 30.0 : 15.0;
        const double gain2n = 20.0;
        uw0029.prepare (sr, GeStageType::UW0029, gainUw, asym, baseSeed + 500);
        n2613.prepare (sr, gain2n, asym, baseSeed + 501);
        updateRIAA();
    }

    void PhonoPreamp::reset()
    {
        uw0029.reset();
        n2613.reset();
        riaaLf.reset();
        riaaHf.reset();
    }

    void PhonoPreamp::setMode (PhonoMode m)
    {
        mode = m;
        updateRIAA();
    }

    void PhonoPreamp::updateRIAA()
    {
        if (mode == PhonoMode::H)
        {
            // RIAA-approximation: LF-shelf @ 50 Hz +20 dB, HF-shelf @ 12 kHz -2 dB
            riaaLf.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
                sampleRate, 50.0f, 0.707f, juce::Decibels::decibelsToGain (20.0f));
            riaaHf.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
                sampleRate, 12000.0f, 0.707f, juce::Decibels::decibelsToGain (-2.0f));
        }
        else  // L = keramisk, nästan flat
        {
            riaaLf.coefficients = new juce::dsp::IIR::Coefficients<float> (
                1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
            riaaHf.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
                sampleRate, 8000.0f, 0.707f, juce::Decibels::decibelsToGain (-1.0f));
        }
    }

    void PhonoPreamp::process (juce::AudioBuffer<float>& buffer, int channel)
    {
        // Pipeline: UW0029 → RIAA → 2N2613
        uw0029.process (buffer, channel);
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
            data[i] = riaaHf.processSample (riaaLf.processSample (data[i]));
        n2613.process (buffer, channel);
    }
}
