/*  TapeSaturation implementation. */

#include "TapeSaturation.h"
#include <cmath>
#include <random>

namespace bc2000dl::dsp
{
    void TapeSaturation::prepare (double sr, std::uint32_t seed)
    {
        sampleRate = sr;
        lcgState = (seed != 0) ? seed : static_cast<std::uint32_t> (std::random_device {} ());

        // 1.5 s print-through-buffer
        printBuffer.assign (static_cast<size_t> (sr * 1.5), 0.0f);
        printIdx = 0;

        // 8× oversampling kring J-A-hysteres-blocket. JUCE FIR Equirripple
        // ger nästan perfekt anti-alias upp till SR_internal/2.
        oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
            1, kOversampleFactor,
            juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple);
        oversampler->initProcessing (4096);  // max block-storlek

        // Bias-rejection LP @ 25 kHz vid oversamplad rate
        const double srOversampled = sr * (1 << kOversampleFactor);
        biasReject.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (
            srOversampled, 25000.0f, 0.707f);
        biasReject.reset();

        hysteresis.reset();
        biasPhase = 0.0;

        updateFilters();
    }

    void TapeSaturation::reset()
    {
        hfFilter.reset();
        bumpFilter.reset();
        std::fill (printBuffer.begin(), printBuffer.end(), 0.0f);
        printIdx = 0;
    }

    void TapeSaturation::setSpeed (TapeSpeed speed)
    {
        currentSpeed = speed;
        updateFilters();
    }

    void TapeSaturation::setBiasAmount (float a)        { biasAmount      = a; }
    void TapeSaturation::setSaturationDrive (float d)   { saturationDrive = d; }
    void TapeSaturation::setPrintThrough (float p)      { printThrough    = p; }
    void TapeSaturation::setFormula (TapeFormula f)
    {
        if (f != formula)
        {
            formula = f;
            updateFilters();
        }
    }

    void TapeSaturation::updateFilters()
    {
        double hfCorner, headBumpFreq, headBumpGainDb, noiseDb;

        switch (currentSpeed)
        {
            case TapeSpeed::Speed19:
                hfCorner = kTapeHF_Speed19;
                headBumpFreq = 70.0;
                // Reducerad från 2.0 → 0.5 dB. Tidigare gav den + playback-EQ:s
                // LF-shelf en samlad +6 dB @ 50 Hz, vilket fail:ade ±3 dB-spec.
                headBumpGainDb = 0.5;
                noiseDb = kTapeNoise_dB_Speed19;
                break;
            case TapeSpeed::Speed95:
                hfCorner = kTapeHF_Speed95;
                headBumpFreq = 80.0;
                headBumpGainDb = 0.7;
                noiseDb = kTapeNoise_dB_Speed95;
                break;
            case TapeSpeed::Speed475:
                hfCorner = kTapeHF_Speed475;
                headBumpFreq = 90.0;
                headBumpGainDb = 1.0;
                noiseDb = kTapeNoise_dB_Speed475;
                break;
        }

        // Tape-formel-justering (plan §7 — typiska 1968-formler).
        // Förstärkta differentialer (v29.8.1) för att vara tydligt hörbara
        // utan ansträngning. Tidigare värden var subtila (±2 dB max);
        // nu får varje formel egen distinkt karaktär.
        switch (formula)
        {
            case TapeFormula::Agfa:
                // Agfa PEM468 — mid-forward, warm, balanced (referens)
                // Lite extra LF-bump för "Visconti '74 Berlin"-känslan
                headBumpGainDb += 0.8;  // mer bass-warmth
                break;
            case TapeFormula::BASF:
                // BASF SPR50LH — bright, clean, modern. Mer HF-extension,
                // tighter low end, lägre brus. "CHROME"-karaktär.
                hfCorner *= 1.40;       // markant brighter (var 1.15)
                headBumpGainDb -= 1.5;  // tighter low (var -0.5)
                noiseDb -= 3.5;         // tystare (var -2)
                break;
            case TapeFormula::Scotch:
                // Scotch 111/202 — dark, compressed, grungy. Markant
                // mörkare HF, fatter low, mer brus. "METAL IV"-karaktär.
                hfCorner *= 0.65;       // markant mörkare (var 0.88)
                headBumpGainDb += 2.5;  // fatter low (var +0.5)
                noiseDb += 3.0;         // mer brus (var +1.5)
                break;
        }

        hfFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (
            sampleRate, static_cast<float> (hfCorner));

        // Head-bump som peaking-EQ
        bumpFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            sampleRate, static_cast<float> (headBumpFreq), 1.5f,
            juce::Decibels::decibelsToGain (static_cast<float> (headBumpGainDb)));

        noiseAmpLin = std::pow (10.0f, static_cast<float> (noiseDb) / 20.0f);
    }

    void TapeSaturation::process (juce::AudioBuffer<float>& buffer, int channel)
    {
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();

        if (! oversampler)
            return;

        // ===== 1. Oversamplad J-A-hysteres med 100 kHz bias som riktig signal =====
        // Wrap nuvarande kanal-data i en single-channel AudioBlock för oversampling
        juce::dsp::AudioBlock<float> singleCh (&data, 1, 0, (size_t) n);
        auto upBlock = oversampler->processSamplesUp (singleCh);

        const int nUp = (int) upBlock.getNumSamples();
        auto* up = upBlock.getChannelPointer (0);
        const double srOver = sampleRate * (1 << kOversampleFactor);
        const double biasInc = juce::MathConstants<double>::twoPi * kBiasFreq_Hz / srOver;
        const float biasAmplitude = 0.03f * biasAmount;
        const float satDrive = saturationDrive;

        for (int i = 0; i < nUp; ++i)
        {
            // Audio + bias-sinus → J-A
            const float biasSig = biasAmplitude * std::sin ((float) biasPhase);
            biasPhase += biasInc;
            if (biasPhase > juce::MathConstants<double>::twoPi)
                biasPhase -= juce::MathConstants<double>::twoPi;

            const float H = up[i] * satDrive + biasSig;
            float fluxOut = hysteresis.processSample (H);

            // LP @ 25 kHz tar bort bias + alias-energi
            fluxOut = biasReject.processSample (fluxOut);
            up[i] = fluxOut;
        }

        oversampler->processSamplesDown (singleCh);

        // ===== 2. Per-formel asymmetrisk + tape-asymmetri =====
        // J-A är 3rd-dominant. Reell tape har även 2nd-harm asymmetri eftersom
        // magnetic flux negativ != positiv. Per-formula skillnad förstärks.
        // Per-formel asymmetri-spread (v29.8.1 — förstärkt från ±0.05 till ±0.15).
        // Scotch = mer 2nd-harmonic crunch, BASF = renare även-symmetrisk distortion,
        // Agfa = balanserad mellan dem.
        const float perFormulaAsym = (formula == TapeFormula::Scotch ?  0.18f :
                                       formula == TapeFormula::BASF   ? -0.10f :
                                                                          0.04f); // Agfa lite varm
        // Generic tape-asymmetri (baseline +0.04 = magnetisk preferens åt positiv flux)
        const float baseAsym = 0.04f;

        for (int i = 0; i < n; ++i)
        {
            float y = data[i];

            // Asymmetric saturation: pos/neg flux har olika expansion-curva
            // Pro-detail: matchar real tape's 2nd-harmonic content
            const float yClamped = juce::jlimit (-1.0f, 1.0f, y);
            y = y * (1.0f + (baseAsym + perFormulaAsym) * yClamped);

            // Subtilt 2nd-harmonic bias (pos > neg compression)
            if (y > 0.0f)
                y = y * (1.0f - 0.015f * y);    // mer kompression på pos
            else
                y = y * (1.0f + 0.005f * y);    // mindre på neg

            // 2. Print-through (om aktiv)
            if (printThrough > 1e-6f)
            {
                const float old = printBuffer[static_cast<size_t> (printIdx)];
                y += old * printThrough;
                printBuffer[static_cast<size_t> (printIdx)] = data[i];
                printIdx = (printIdx + 1) % static_cast<int> (printBuffer.size());
            }

            // 3. HF-roll-off (per hastighet)
            y = hfFilter.processSample (y);

            // 4. Head-bump
            y = bumpFilter.processSample (y);

            // 5. Tape-egenbrus (LCG Gaussian — RT-safe, 30× snabbare än mt19937)
            y += detail::fastGaussNoise (lcgState) * noiseAmpLin;

            data[i] = y;
        }
    }
}
