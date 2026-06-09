/*  WowFlutter v2 implementation — multikomponent + stokastisk drift + scrape. */

#include "WowFlutter.h"
#include <cmath>

namespace bc2000dl::dsp
{
    namespace
    {
        // Pitch-% → delay-amplitud i samples:  p = 2π·f·D  →  D = p/(2π·f)
        // (p som andel, D i sekunder; ×sr ger samples).
        float ampFromPitchPct (float pct, float freqHz, double sr)
        {
            return (pct / 100.0f)
                 / (juce::MathConstants<float>::twoPi * freqHz)
                 * static_cast<float> (sr);
        }

        inline float lcgNoise (std::uint32_t& state)
        {
            state = state * 1664525u + 1013904223u;
            return static_cast<float> (static_cast<std::int32_t> (state))
                 * (1.0f / 2147483648.0f);   // uniform ±1
        }
    }

    void WowFlutter::prepare (double sr)
    {
        sampleRate = sr;

        // Max modulationsdjup: värsta fallet (4,75 cm/s) summerar till
        // ~25 samples @ 48 kHz vid amount=1; ×2 (amount-max) ×1,3 (AM-jitter)
        // ≈ 65. 1,4e-3·sr ger marginal. Hela delayn är fast och PDC-
        // rapporteras via getLatencySamples().
        const int maxMod = static_cast<int> (std::ceil (sr * 1.4e-3)) + 1;
        baseDelay = maxMod + 8;
        buf.assign (static_cast<size_t> (baseDelay + maxMod + 8), 0.0f);
        writeIdx = 0;

        // AM/FM-jitter: 1-pol LP på uniformt ±1-brus. Normalisera RMS → 1
        // så djupet styrs explicit:  RMS_lp = sqrt(a/(2−a)) · RMS_in.
        amCoeff = juce::MathConstants<float>::twoPi * 0.15f / static_cast<float> (sr);
        fmCoeff = juce::MathConstants<float>::twoPi * 0.40f / static_cast<float> (sr);
        const float rmsIn = 1.0f / std::sqrt (3.0f);   // uniform ±1
        amGain = 1.0f / (std::sqrt (amCoeff / (2.0f - amCoeff)) * rmsIn);
        fmGain = 1.0f / (std::sqrt (fmCoeff / (2.0f - fmCoeff)) * rmsIn);

        // Scrape-BP: 1-pol HP @ 1,8 kHz + 1-pol LP @ 3,8 kHz ≈ band kring 2,7 kHz
        scrapeHpCoeff = juce::MathConstants<float>::twoPi * 1800.0f / static_cast<float> (sr);
        scrapeLpCoeff = juce::jmin (0.9f,
            juce::MathConstants<float>::twoPi * 3800.0f / static_cast<float> (sr));

        reset();
        rebuildComponents();
    }

    void WowFlutter::reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        writeIdx = 0;
        for (auto& s : sines) s.phase = 0.0f;
        amState = fmState = 0.0f;
        jitterSeed  = 0x1968BEEFu;
        scrapeState = scrapeSeed;
        scrapeBpLo = scrapeBpHi = 0.0f;
    }

    void WowFlutter::setSpeed (TapeSpeed speed)
    {
        currentSpeed = speed;
        rebuildComponents();
    }

    void WowFlutter::setStereoRole (float sign, std::uint32_t seed)
    {
        weaveSign  = (sign < 0.0f) ? -1.0f : 1.0f;
        scrapeSeed = (seed != 0) ? seed : 0xC0FFEEu;
        scrapeState = scrapeSeed;
    }

    void WowFlutter::rebuildComponents()
    {
        // Totalbudget (pitch-%) per hastighet — samma kalibrering som spec §7:
        //   Wow:     19 cm/s 0,075 %   9,5 cm/s 0,125 %   4,75 cm/s 0,200 %
        //   Flutter: 19 cm/s 0,065 %   9,5 cm/s 0,100 %   4,75 cm/s 0,160 %
        // Rotationsfrekvenser ur mekaniken (f = v/(π·d)):
        //   pinch-roller d≈13 mm, capstan d≈4,76 mm. Motor/idler är
        //   hastighetsoberoende (motorvarvtal 1425 rpm @ 50 Hz nät → 23,8 Hz).
        float wowPct, flutPct, pinchHz, capstanHz;
        switch (currentSpeed)
        {
            case TapeSpeed::Speed19:
                wowPct = 0.075f; flutPct = 0.065f; pinchHz = 4.65f; capstanHz = 12.7f; break;
            case TapeSpeed::Speed95:
                wowPct = 0.125f; flutPct = 0.100f; pinchHz = 2.32f; capstanHz = 6.35f; break;
            case TapeSpeed::Speed475:
            default:
                wowPct = 0.200f; flutPct = 0.160f; pinchHz = 1.16f; capstanHz = 3.18f; break;
        }

        const auto set = [this] (int i, float freqHz, float pct, float fmDepth)
        {
            sines[i].freqHz     = freqHz;
            sines[i].ampSamples = ampFromPitchPct (pct, freqHz, sampleRate);
            sines[i].fmDepth    = fmDepth;
        };

        // Wow-budget: drift 30 %, pinch 45 %, capstan 25 %.
        // Drift får kraftig FM-jitter (slumpvandring snarare än ton).
        set (0, 0.30f,     wowPct * 0.30f, 0.60f);   // drift
        set (1, pinchHz,   wowPct * 0.45f, 0.06f);   // pinch-roller
        set (2, capstanHz, wowPct * 0.25f, 0.04f);   // capstan
        // Tape-weave: 10 % av wow-budgeten, motfas L/R (weaveSign).
        set (kWeaveIdx, 0.80f, wowPct * 0.10f, 0.30f);
        // Flutter-budget: motor 60 %, idler 25 % (icke-harmoniskt förhållande).
        set (4, 23.8f,     flutPct * 0.60f, 0.03f);  // motorns once-around
        set (5, 37.3f,     flutPct * 0.25f, 0.05f);  // idler/mellanhjul
        // Scrape: ~15 % av flutter-budgeten vid 2,7 kHz. BP-brusets RMS efter
        // HP+LP-kaskaden är ~0,25 av vitt ±1-brus → kompensera ×4.
        scrapeAmp = ampFromPitchPct (flutPct * 0.15f, 2700.0f,  sampleRate) * 4.0f;
    }

    float WowFlutter::processSample (float x)
    {
        if (buf.empty()) return x;

        buf[static_cast<size_t> (writeIdx)] = x;
        writeIdx = (writeIdx + 1) % static_cast<int> (buf.size());

        // Långsamt AM/FM-jitter — gör komponenterna till smala band
        // ("kjolar") i stället för rena spektrallinjer. Samma deterministiska
        // sekvens i L och R (delat fysiskt band).
        const float jn = lcgNoise (jitterSeed);
        amState += amCoeff * (jn - amState);
        fmState += fmCoeff * (jn - fmState);
        const float amFac = 1.0f + 0.20f * (amState * amGain);

        // Summera sinuskomponenterna
        float mod = 0.0f;
        for (int i = 0; i < kNumSines; ++i)
        {
            auto& s = sines[i];
            const float a = s.ampSamples * amFac
                          * (i == kWeaveIdx ? weaveSign : 1.0f);
            mod += a * std::sin (s.phase);
            const float fJit = 1.0f + s.fmDepth * (fmState * fmGain);
            s.phase += juce::MathConstants<float>::twoPi * s.freqHz * fJit
                     / static_cast<float> (sampleRate);
            if (s.phase > juce::MathConstants<float>::twoPi)
                s.phase -= juce::MathConstants<float>::twoPi;
            else if (s.phase < 0.0f)
                s.phase += juce::MathConstants<float>::twoPi;
        }

        // Scrape flutter — per-kanal BP-brus (stick-slip mot huvudet)
        const float sn = lcgNoise (scrapeState);
        scrapeBpLo += scrapeHpCoeff * (sn - scrapeBpLo);          // LP-del till HP
        const float hp = sn - scrapeBpLo;
        scrapeBpHi += scrapeLpCoeff * (hp - scrapeBpHi);          // LP ovanpå
        mod += scrapeAmp * scrapeBpHi;

        // Ingen early-return på amount — delayvägen är alltid aktiv så
        // latensen är konstant (PDC-korrekt) även vid automation genom noll.
        const float delaySamps = juce::jlimit (
            3.0f, static_cast<float> (buf.size()) - 2.0f,
            static_cast<float> (baseDelay) + mod * amount);

        // Lagrange 3:e-ordningens interpolation
        const int idxInt = static_cast<int> (std::floor (delaySamps));
        const float frac = delaySamps - static_cast<float> (idxInt);

        const int sz = static_cast<int> (buf.size());
        const int i0 = ((writeIdx - idxInt - 1) % sz + sz) % sz;
        const int i1 = ((writeIdx - idxInt    ) % sz + sz) % sz;
        const int i2 = ((writeIdx - idxInt + 1) % sz + sz) % sz;
        const int i3 = ((writeIdx - idxInt + 2) % sz + sz) % sz;
        const float x0 = buf[static_cast<size_t> (i0)];
        const float x1 = buf[static_cast<size_t> (i1)];
        const float x2 = buf[static_cast<size_t> (i2)];
        const float x3 = buf[static_cast<size_t> (i3)];

        const float c0 = -frac * (frac - 1.0f) * (frac - 2.0f) / 6.0f;
        const float c1 = (frac + 1.0f) * (frac - 1.0f) * (frac - 2.0f) / 2.0f;
        const float c2 = -(frac + 1.0f) * frac * (frac - 2.0f) / 2.0f;
        const float c3 = (frac + 1.0f) * frac * (frac - 1.0f) / 6.0f;

        return c0 * x0 + c1 * x1 + c2 * x2 + c3 * x3;
    }

    void WowFlutter::process (juce::AudioBuffer<float>& buffer, int channel)
    {
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
            data[i] = processSample (data[i]);
    }
}
