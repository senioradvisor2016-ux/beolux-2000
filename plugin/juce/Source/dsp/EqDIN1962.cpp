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
    // DIN 1962 / DIN 45513 reel-to-reel time constants per B&O service-manualen
    // sida 2: "Einspielkennlinie: nach DIN 1962".  Treble-tidskonstanter:
    //   19   cm/s:  70 µs  →  fc = 1/(2π·70µs)  = 2274 Hz
    //    9,5 cm/s:  90 µs  →  fc = 1/(2π·90µs)  = 1768 Hz
    //    4,75 cm/s: 120 µs →  fc = 1/(2π·120µs) = 1326 Hz
    // Bass: 3180 µs (50 Hz) — gemensam för alla hastigheter.
    //
    // Tidigare corners (12k/7k5/4k Hz) var helt fel — de satte de-emphasis-shelf
    // OVANFÖR själva tape-bandbredden, så i tape-bandet (1–8 kHz) hade EQ:n
    // ingen effekt.  Det är därför §6 "tape glow" uppmättes som DIP istället
    // för PEAK i tester före v62.0.
    void PlaybackEqDIN1962::setSpeed (TapeSpeed speed)
    {
        Config cfg;
        switch (speed)
        {
            case TapeSpeed::Speed19:
                cfg = { 50.0f, 5.0f, 2274.0f, -10.0f }; break;
            case TapeSpeed::Speed95:
                cfg = { 50.0f, 6.0f, 1768.0f, -13.0f }; break;
            case TapeSpeed::Speed475:
                cfg = { 50.0f, 9.0f, 1326.0f, -16.0f }; break;
        }
        setConfig (cfg);
    }

    // ---------- PreEmphasisDIN1962 ----------
    // Pre-emphasis-curvan är medvetet "över-kompenserad" jämfört med playback —
    // mismatchen ger den karaktäristiska "tape glow"/presence-peak som spec §6
    // efterfrågar (+3/+5/+6 dB i 5–8/4–7/3–5 kHz beroende på hastighet).
    //
    // Net mismatch ovanför corner = pre_gain - |de_gain|:
    //   19   cm/s:  +14 - 10 = +4  dB rest-gain ovanför 2274 Hz
    //    9,5 cm/s:  +18 - 12 = +6  dB rest-gain ovanför 1768 Hz
    //    4,75 cm/s: +22 - 14 = +8  dB rest-gain ovanför 1326 Hz
    void PreEmphasisDIN1962::setSpeed (TapeSpeed speed)
    {
        Config cfg;
        switch (speed)
        {
            case TapeSpeed::Speed19:
                cfg = { 0.0f, 0.0f, 2274.0f, 14.0f }; break;
            case TapeSpeed::Speed95:
                cfg = { 0.0f, 0.0f, 1768.0f, 20.0f }; break;
            case TapeSpeed::Speed475:
                cfg = { 0.0f, 0.0f, 1326.0f, 30.0f }; break;
        }
        setConfig (cfg);
    }
}
