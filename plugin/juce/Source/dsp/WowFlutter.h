/*  WowFlutter — fysikaliskt motiverad pitch-modulation via delay-line (v2).

    Verklig bandgång är ett spektrum, inte två rena sinusar:

      · Drift        (~0,3 Hz)  — bandspänning/reel-servo, slumpvandrande
      · Pinch-roller (1,2–4,7 Hz beroende på hastighet) — once-around
      · Capstan      (3,2–12,7 Hz beroende på hastighet) — once-around
      · Tape-weave   (~0,8 Hz)  — azimut-vandring, MOTFAS L/R
      · Motor        (23,8 Hz)  — 4-pol asynkronmotor @ 50 Hz nät (1968 DK)
      · Idler        (37,3 Hz)  — mellanhjul, icke-harmoniskt förhållande
      · Scrape       (~2,7 kHz) — stick-slip mot huvuden, BP-filtrerat brus

    Rotationsfrekvenserna skalar med bandhastigheten (mekanik: f = v/(π·d)),
    motor/idler är hastighetsoberoende (samma motorvarvtal, annan utväxling).
    Alla deterministiska komponenter moduleras långsamt i amplitud och
    frekvens (1-pol LP-brus) — verkliga W&F-spektra är smala band med
    "kjolar", inte rena linjer.

    L/R: pitch-modulationen är i grunden KORRELERAD (samma fysiska band
    genom samma capstan) — sinuskomponenterna och deras jitter kör samma
    deterministiska sekvens i båda kanalerna. Det som skiljer: scrape
    (per-spår stick-slip, oberoende brus-seed) och tape-weave (motfas).

    Basdelayn är fast och PDC-rapporteras via getLatencySamples().
    Lagrange 3:e ordningens interpolation på läspekaren.

    Plats: plugin/juce/Source/dsp/WowFlutter.h
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Constants.h"

namespace bc2000dl::dsp
{
    class WowFlutter
    {
    public:
        WowFlutter() = default;

        void prepare (double sampleRate);
        void reset();
        void setSpeed (TapeSpeed speed);
        void setAmount (float a) { amount = juce::jlimit (0.0f, 2.0f, a); }

        // Stereoroll: weaveSign ±1 (tape-weave i motfas L/R) och per-kanal
        // brus-seed för scrape. AM/FM-jittret seedas LIKA i båda kanalerna
        // (delat fysiskt band) — bara scrape decorreleras.
        void setStereoRole (float weaveSign, std::uint32_t scrapeSeed);

        // Fast basdelay (alltid aktiv, oavsett amount) — rapporteras som
        // plugin-latens via setLatencySamples() så värden kan PDC-kompensera.
        int getLatencySamples() const { return baseDelay; }

        float processSample (float x);
        void process (juce::AudioBuffer<float>& buffer, int channel);

    private:
        // Sinuskomponenter: drift, pinch, capstan, weave, motor, idler
        static constexpr int kNumSines = 6;
        static constexpr int kWeaveIdx = 3;   // motfas-komponenten

        struct Sine
        {
            float freqHz { 0.0f };
            float ampSamples { 0.0f };   // delay-amplitud i samples (amount=1)
            float fmDepth { 0.0f };      // relativ frekvensjitter (0..1)
            float phase { 0.0f };
        };

        void rebuildComponents();        // freq/amp per hastighet + samplerate

        double sampleRate { 48000.0 };
        float amount { 1.0f };
        TapeSpeed currentSpeed { TapeSpeed::Speed19 };
        float weaveSign { 1.0f };

        Sine sines[kNumSines];

        // Delat långsamt AM/FM-jitter (1-pol LP på LCG-brus, samma seed L/R)
        std::uint32_t jitterSeed { 0x1968BEEFu };
        float amState { 0.0f }, fmState { 0.0f };
        float amCoeff { 0.0f }, fmCoeff { 0.0f };
        float amGain { 0.0f },  fmGain { 0.0f };   // normaliserar LP-brusets RMS

        // Scrape flutter: per-kanal brus → 2-pol BP @ ~2,7 kHz
        std::uint32_t scrapeSeed { 0xC0FFEEu }, scrapeState { 0xC0FFEEu };
        float scrapeAmp { 0.0f };
        float scrapeBpLo { 0.0f }, scrapeBpHi { 0.0f };  // HP+LP-kaskadtillstånd
        float scrapeLpCoeff { 0.0f }, scrapeHpCoeff { 0.0f };

        // Ring-buffer för delay-line. Dimensioneras i prepare() till
        // max modulationsdjup + marginal (~1,5 ms @ 48 kHz).
        std::vector<float> buf;
        int writeIdx  { 0 };
        int baseDelay { 0 };
    };
}
