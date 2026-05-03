/*  PhonoPreamp implementation. */

#include "PhonoPreamp.h"

namespace bc2000dl::dsp
{
    void PhonoPreamp::prepare (double sr, float asym, std::uint32_t baseSeed)
    {
        sampleRate = sr;
        // Plugin-kontext: input är redan line-level (DAW), inte phono-cartridge
        // 2 mV. Reducerar gain-cascaden från fysisk-hårdvara-värdena (70 dB i H,
        // 35 dB i L) till plugin-värden som balanserar mot mic/radio (6 dB).
        // Behåller RIAA-shape för phono-karaktär utan att överamplifiera.
        // H-läge: ~9 dB midband + 6 dB LF-shelf = ~15 dB @ LF
        // L-läge: ~6 dB total (matchar mic/radio)
        const double gainUw = (mode == PhonoMode::H) ?  6.0 : 4.0;
        const double gain2n = (mode == PhonoMode::H) ?  3.0 : 2.0;
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

    void PhonoPreamp::setChannelAsymmetry (float offset)
    {
        uw0029.setChannelAsymmetry (static_cast<double> (offset));
        n2613.setChannelAsymmetry  (static_cast<double> (offset));
    }

    void PhonoPreamp::updateRIAA()
    {
        if (mode == PhonoMode::H)
        {
            // RIAA-approximation reducerad till plugin-skala: behåller phono-färg
            // (LF-warmth, slight HF roll-off) utan att över-boosta.
            // LF-shelf @ 50 Hz +6 dB (var +20), HF-shelf @ 12 kHz -2 dB.
            riaaLf.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
                sampleRate, 50.0f, 0.707f, juce::Decibels::decibelsToGain (6.0f));
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
