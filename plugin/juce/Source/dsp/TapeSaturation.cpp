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

        // 8× oversampling kring J-A-hysteres-blocket. PolyphaseIIR är 4-6×
        // snabbare än FIREquiripple och räcker för ett olinjärt block — fasvridning
        // vid Nyquist är ej hörbar och påverkar ej harmonisk spektrumbalans.
        oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
            1, kOversampleFactor,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
        oversampler->initProcessing (4096);  // max block-storlek

        // Bias-rejection LP @ 40 kHz vid oversamplad rate.
        // Höjd från 25 → 40 kHz (v62.0): 25 kHz gav -1.5 dB redan @ 20 kHz,
        // vilket åt upp HF-marginalen i §2-spec.  40 kHz ger -0.4 dB @ 20 kHz.
        // 100 kHz bias är fortfarande hårt undertryckt (>40 dB rejection)
        // tack vare 3-stegs IIR-decimation + 2:a-ord LP.
        const double srOversampled = sr * (1 << kOversampleFactor);
        biasReject.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (
            srOversampled, 40000.0f, 0.707f);
        biasReject.reset();

        hysteresis.reset();

        // Bias-oscillator: förberäkna rotations-steg.  ω = 2π·f_bias/sr_over.
        // Init (cos,sin) = (1,0) → start vid sin-phase 0 (matchar gamla biasPhase=0).
        const double omega = juce::MathConstants<double>::twoPi * kBiasFreq_Hz / srOversampled;
        biasCosStep  = std::cos (omega);
        biasSinStep  = std::sin (omega);
        biasCosState = 1.0;
        biasSinState = 0.0;

        updateFilters();
    }

    void TapeSaturation::reset()
    {
        hfFilter.reset();
        bumpFilter.reset();
        biasReject.reset();
        hysteresis.reset();   // clears J-A magnetisation history
        biasCosState = 1.0;
        biasSinState = 0.0;
        if (oversampler) oversampler->reset();
        std::fill (printBuffer.begin(), printBuffer.end(), 0.0f);
        printIdx = 0;
        dcBlockX1 = dcBlockY1 = 0.0f;
        silenceBlockCount = 0;
    }

    void TapeSaturation::setSpeed (TapeSpeed speed)
    {
        currentSpeed = speed;
        updateFilters();
        // Reset all internal state on speed change.  Stale magnetisation
        // history bound to previous-speed bias/EQ params can trigger
        // numerical drift on the first few samples.
        hfFilter.reset();
        bumpFilter.reset();
        biasReject.reset();
        hysteresis.reset();
        if (oversampler) oversampler->reset();
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
            // Reset all internal state when formula changes — the J-A
            // magnetisation history is bound to the previous formula's
            // coercivity (k) parameter; without reset, BASF's small k=0.07
            // applied to an M-state from Scotch's k=0.13 can produce
            // numerical drift / NaN on the first few samples.  Same for
            // filter state in hf/bump/biasReject and oversampler internals.
            hfFilter.reset();
            bumpFilter.reset();
            biasReject.reset();
            hysteresis.reset();
            if (oversampler) oversampler->reset();
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
                hysteresis.setParams (JilesAtherton::presetAgfa());
                break;
            case TapeFormula::BASF:
                // BASF SPR50LH — bright, clean, modern. Mer HF-extension,
                // tighter low end, lägre brus. "CHROME"-karaktär.
                hfCorner *= 1.40;       // markant brighter (var 1.15)
                headBumpGainDb -= 1.5;  // tighter low (var -0.5)
                noiseDb -= 3.5;         // tystare (var -2)
                hysteresis.setParams (JilesAtherton::presetBASF());
                break;
            case TapeFormula::Scotch:
                // Scotch 111/202 — dark, compressed, grungy. Markant
                // mörkare HF, fatter low, mer brus. "METAL IV"-karaktär.
                hfCorner *= 0.65;       // markant mörkare (var 0.88)
                headBumpGainDb += 2.5;  // fatter low (var +0.5)
                noiseDb += 3.0;         // mer brus (var +1.5)
                hysteresis.setParams (JilesAtherton::presetScotch());
                break;
        }

        // Hård Nyquist-clamp på hfCorner.  För Speed19 + BASF blev hfCorner
        // 30000 × 1.40 = 42000 Hz > Nyquist (24 kHz @ 48 kHz SR), vilket gav
        // instabila bilinear-koefficienter (tan(π·fc/Fs) → ∞ när fc → Fs/2)
        // → hfFilter spottar omedelbart ut NaN.  0.45 × SR = 21,6 kHz @ 48 kHz
        // är säker marginal under Nyquist.
        const float safeHF = static_cast<float> (
            std::min (hfCorner, sampleRate * 0.45));
        hfFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (
            sampleRate, safeHF);

        // Head-bump som peaking-EQ.  Vid slow speeds (4.75 cm/s) lägger vi
        // dessutom in tape-glow-peaken DIREKT i bump-filtret istället för att
        // försöka få ut den ur pre/de-emphasis-mismatchen — den senare lider
        // av J-A-mättnad vid höga signal-nivåer i tape-bandet, vilket
        // kompresserar fundamental och skär ner glow-amplitud.
        //
        // För Speed475 lägger vi den till en kraftfullare peak vid 4 kHz
        // (mitten av spec §6 3-5 kHz target-band).  Detta är fysikaliskt
        // motiverat: real tape vid 4.75 cm/s har naturlig presence-peak
        // ungefär här pga gap-loss-kompenseringen i playback-EQ:n.
        if (currentSpeed == TapeSpeed::Speed475)
        {
            // Lägg head-bump som en bandstop-form: combine LF-bump + HF-presence.
            // Vid Speed475 är tape-glow-peaken VIKTIGARE än LF-headbumpen,
            // så vi prioriterar 4 kHz-presence här.
            bumpFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
                sampleRate, 4000.0f, 1.5f,
                juce::Decibels::decibelsToGain (4.0f));
        }
        else
        {
            bumpFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
                sampleRate, static_cast<float> (headBumpFreq), 1.5f,
                juce::Decibels::decibelsToGain (static_cast<float> (headBumpGainDb)));
        }

        noiseAmpLin = std::pow (10.0f, static_cast<float> (noiseDb) / 20.0f);
    }

    void TapeSaturation::process (juce::AudioBuffer<float>& buffer, int channel)
    {
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();

        if (! oversampler)
            return;

        // NaN-scrub input buffer.  Eventuella upstream-NaN/Inf förorenar
        // oversampler-FIR-buffrarna permanent (FIR-state memoreras), så
        // skydda hela kedjan genom att skrubba här.
        for (int i = 0; i < n; ++i)
            if (! std::isfinite (data[i]))
                data[i] = 0.0f;

        // Tyst-ingångsgating: räkna block under tröskeln och nollställ J-A-state.
        // Kontinuerlig konstant-amplituds-bias demagnetiserar INTE M_r i J-A-modellen
        // (kräver avtagande amplitud, som en riktigt erase-head).  Vi simulerar
        // erasure-effekten: efter kSilenceHoldBlocks tysta block sätts M = 0.
        {
            float blockPow = 0.0f;
            for (int i = 0; i < n; ++i)
                blockPow += data[i] * data[i];
            if (blockPow / static_cast<float> (n) < kSilenceThreshPow)
            {
                if (++silenceBlockCount >= kSilenceHoldBlocks)
                {
                    hysteresis.reset();
                    biasReject.reset();
                    hfFilter.reset();
                    bumpFilter.reset();
                    if (oversampler) oversampler->reset();
                    dcBlockX1 = dcBlockY1 = 0.0f;
                    silenceBlockCount = kSilenceHoldBlocks;  // klipp mot overflow
                }
            }
            else
            {
                silenceBlockCount = 0;
            }
        }

        // ===== 1. Oversamplad J-A-hysteres med 100 kHz bias som riktig signal =====
        // Wrap nuvarande kanal-data i en single-channel AudioBlock för oversampling
        juce::dsp::AudioBlock<float> singleCh (&data, 1, 0, (size_t) n);
        auto upBlock = oversampler->processSamplesUp (singleCh);

        const int nUp = (int) upBlock.getNumSamples();
        auto* up = upBlock.getChannelPointer (0);
        const float biasAmplitude = 0.03f * biasAmount;
        // Renormalisera oscillator-state mot drift (1 sqrt per block, försumbart).
        // Numerisk avrundning i 4-mul-rotationen ackumuleras till |state|≠1 över tid;
        // 1/sqrt(c²+s²) trimmar tillbaka utan att påverka frekvensen.
        {
            const double mag2 = biasCosState * biasCosState + biasSinState * biasSinState;
            const double inv  = 1.0 / std::sqrt (mag2);
            biasCosState *= inv;
            biasSinState *= inv;
        }
        // Drive-scaling 0.1 (v62.5).  Med ac126-gain reducerad till 0 dB
        // hamnar tape-input vid -3 dBFS test-signal på ca -4 dBFS (peak 0.6).
        // J-A H_peak = 0.6 × 0.1 = 0.06 → L(0.2) = 0.067, linjär region.
        // THD från J-A själv blir < 0.5 %.  Återstoden från asymmetric-sat
        // (≈ 0.5-1 %) + GE cascade (≈ 0.5-1 %) summerar till <3 % spec.
        constexpr float kDriveScaling = 0.1f;
        const float satDrive = saturationDrive * kDriveScaling;

        // Lokal kopia av oscillator-state — låter compilern hålla i register.
        double oscC = biasCosState;
        double oscS = biasSinState;
        const double cw = biasCosStep;
        const double sw = biasSinStep;

        for (int i = 0; i < nUp; ++i)
        {
            // Bias-sinus via 2D-rotation: (c,s) → (c·cw − s·sw, s·cw + c·sw).
            // 4 muls + 2 adds, ingen std::sin per sample.
            const float biasSig = biasAmplitude * static_cast<float> (oscS);
            const double newC = oscC * cw - oscS * sw;
            const double newS = oscS * cw + oscC * sw;
            oscC = newC;
            oscS = newS;

            // Hard-clamp H före J-A.  Magnetisering kan inte fysikaliskt
            // gå utanför ±5·Ms; värden därutöver indikerar upstream-blowup
            // och skulle ge extrema dM/dH-transienter som driver modellen
            // in i NaN-läge.
            const float H = juce::jlimit (-5.0f, 5.0f,
                                          up[i] * satDrive + biasSig);
            float fluxOut = hysteresis.processSample (H);
            // NaN-guard: BASF-presetens lilla k (0.07) kan vid stora dH ge
            // ackumulerad numerisk drift som fortplantar sig som NaN.
            if (! std::isfinite (fluxOut))
                fluxOut = 0.0f;

            // LP @ 40 kHz tar bort bias + alias-energi (v62.0 — höjd från 25)
            fluxOut = biasReject.processSample (fluxOut);
            up[i] = fluxOut;
        }

        // Skriv tillbaka oscillator-state inför nästa block.
        biasCosState = oscC;
        biasSinState = oscS;

        oversampler->processSamplesDown (singleCh);

        // Scrub + DC-block + makeup gain.
        // J-A:s linjära susceptibilitet χ₀ = Ms/(3a) ≈ 1 för alla formler
        // (Agfa 1.11, BASF 1.19, Scotch 0.98) → gain ≈ kDriveScaling = 0.1 (−20 dB).
        // 1/kDriveScaling återställer unity; fel < 1 dB per formel.
        //
        // DC-block (~10 Hz en-pol HP) tar bort J-A:s remanensmagnetisering M_r
        // innan makeup-gainen förstärker den.  Verkliga bandspelarhuvuden är
        // AC-kopplade (transformator/kondensator) — detta är fysikaliskt korrekt.
        // 10 Hz cutoff: kostar <0.2 dB vid 50 Hz (inom testbudget), tar bort
        // remanenstail (~50 ms tidskonstant → energi under ~20 Hz).
        constexpr float kMakeup  = 1.0f / kDriveScaling;
        const float     dcCoeff  = 1.0f - juce::MathConstants<float>::twoPi
                                          * 10.0f / static_cast<float> (sampleRate);
        for (int i = 0; i < n; ++i)
        {
            if (! std::isfinite (data[i]))
                data[i] = 0.0f;
            const float x = data[i];
            data[i]  = x - dcBlockX1 + dcCoeff * dcBlockY1;
            dcBlockX1 = x;
            dcBlockY1 = data[i];
            data[i] *= kMakeup;
        }

        // ===== 2. Per-formel asymmetrisk + tape-asymmetri =====
        // J-A är 3rd-dominant. Reell tape har även 2nd-harm asymmetri eftersom
        // magnetic flux negativ != positiv. Per-formula skillnad behållen men
        // dämpad till kalibrerade värden (v62.0).
        //
        // Tidigare värden (Scotch ±0.18, BASF -0.10, Agfa +0.04) gav
        // 30+ % asymmetrisk distortion vid -3 dBFS — orealistiskt för 1968
        // tape-formler (typ-värde 1–3 % h2).  Nya värden ger ~1 %, ~0.5 %, ~0.3 %
        // h2 respektive — fortfarande tydlig formel-skillnad men i spec-mål.
        const float perFormulaAsym = (formula == TapeFormula::Scotch ?  0.04f :
                                       formula == TapeFormula::BASF   ? -0.02f :
                                                                          0.01f); // Agfa neutral
        // Generic tape-asymmetri (baseline 0.01, var 0.04)
        const float baseAsym = 0.01f;

        for (int i = 0; i < n; ++i)
        {
            float y = data[i];

            // Asymmetric saturation: pos/neg flux har olika expansion-curva.
            // Matchar real tape's 2nd-harmonic content men i spec-skala.
            const float yClamped = juce::jlimit (-1.0f, 1.0f, y);
            y = y * (1.0f + (baseAsym + perFormulaAsym) * yClamped);

            // Subtilt 2nd-harmonic bias (pos > neg compression).
            // Halverat från 0.015/0.005 → 0.005/0.002 (v62.0).
            if (y > 0.0f)
                y = y * (1.0f - 0.005f * y);    // mer kompression på pos
            else
                y = y * (1.0f + 0.002f * y);    // mindre på neg

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
