/*  Echo implementation. */

#include "Echo.h"
#include <cmath>

namespace bc2000dl::dsp
{
    void Echo::prepare (double sr)
    {
        sampleRate = sr;
        const int maxDelay = static_cast<int> (sr * 0.35);  // 350 ms max
        buf.assign (static_cast<size_t> (maxDelay), 0.0f);
        writeIdx = 0;

        hfLossFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (
            sr, 8000.0f);
        hfLossFilter.reset();

        setSpeed (TapeSpeed::Speed19);
    }

    void Echo::reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        writeIdx = 0;
        hfLossFilter.reset();
        echoWowPhase = 0.0f;
        delaySmoothed = static_cast<float> (delaySamples);  // ingen glide vid reset
    }

    void Echo::setSpeed (TapeSpeed speed)
    {
        switch (speed)
        {
            case TapeSpeed::Speed19:  delayMs = static_cast<float> (kEchoTime_ms_Speed19);  break;
            case TapeSpeed::Speed95:  delayMs = static_cast<float> (kEchoTime_ms_Speed95);  break;
            case TapeSpeed::Speed475: delayMs = static_cast<float> (kEchoTime_ms_Speed475); break;
        }
        delaySamples = std::min (
            static_cast<int> (sampleRate * delayMs / 1000.0f),
            static_cast<int> (buf.size()) - 1);

        // Wow depth: slower tape → less mechanical stability → more pitch-wander in echo
        // HF loss LP: matches tape bandwidth per speed (echo re-records through tape path)
        float hfLossHz;
        switch (speed)
        {
            case TapeSpeed::Speed19:
                echoWowDepth = delaySamples * 0.00050f;
                hfLossHz = 10000.0f;   // 19 cm/s: tape to 20 kHz, loss to ~10 kHz per pass
                break;
            case TapeSpeed::Speed95:
                echoWowDepth = delaySamples * 0.00080f;
                hfLossHz = 7000.0f;    // 9.5 cm/s: tape to 12 kHz, loss to ~7 kHz per pass
                break;
            case TapeSpeed::Speed475:
                echoWowDepth = delaySamples * 0.00120f;
                hfLossHz = 4500.0f;    // 4.75 cm/s: tape to 6 kHz, loss to ~4.5 kHz per pass
                break;
        }
        hfLossFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (
            sampleRate, hfLossHz);
    }

    void Echo::setTimeMs (float ms)
    {
        // DELUXE ECHO TIME — override auto-från-speed-delayen.  Speed:s
        // setSpeed() sätter wow-djup + HF-loss (karaktär); denna sätter längden.
        delayMs = juce::jlimit (30.0f, 350.0f, ms);
        delaySamples = std::min (
            static_cast<int> (sampleRate * delayMs / 1000.0f),
            static_cast<int> (buf.size()) - 1);
    }

    float Echo::processSample (float x)
    {
        if (! enabled || amount < 1e-6f)
            return x;

        // Self-oscillation-mappning (manual §d-varning):
        //   amount  0.0…0.70  → fb 0.0…0.78   (avtagande echo)
        //   amount  0.70…0.85 → fb 0.78…0.96  (långa, men avtagande)
        //   amount  0.85…1.00 → fb 0.96…1.04  (sustained → divergent self-osc)
        // Tape-loop-recordens self-osc kommer från att fb går just över unity.
        // För DSP-stabilitet kläms den hårda klippen via tanh på återmatningen.
        const float fbCurve = amount * (0.92f + 0.12f * amount);  // mjuk ramp till ~1.04
        // DELUXE FEEDBACK-knob: feedbackParam >= 0 åsidosätter auto-kurvan.
        const float feedback = (feedbackParam >= 0.0f)
                             ? juce::jmin (feedbackParam, 1.04f)
                             : juce::jmin (fbCurve, 1.04f);

        // Wow-modulated read pointer — tape echo tails pitch-wander at ~1.5 Hz
        echoWowPhase += juce::MathConstants<float>::twoPi * echoWowFreqHz
                        / static_cast<float> (sampleRate);
        if (echoWowPhase >= juce::MathConstants<float>::twoPi)
            echoWowPhase -= juce::MathConstants<float>::twoPi;

        // Glid delay-längden mjukt mot målet — annars hoppar läspekaren när
        // TIME-knoben vrids → knaster/klick. Mjuk ramp (~30 ms) ger istället en
        // autentisk tape-pitch-glide. ~0.0007 ≈ 1-pol @ 48 kHz.
        delaySmoothed += (static_cast<float> (delaySamples) - delaySmoothed) * 0.0007f;

        const float wowOffset  = echoWowDepth * std::sin (echoWowPhase);

        // v60.5 — Cross-feedback (SoS): läs delay-sample från partner-Echo:s
        // buffer istället för egen.  Resultatet: L:s echo-output har R:s
        // delayed-signal med feedback, R:s output har L:s.  Ping-pong/X-feed.
        const std::vector<float>& readBuf = (feedbackSource != nullptr)
                                          ? feedbackSource->getDelayBuffer()
                                          : buf;
        const int bufLen = static_cast<int> (readBuf.size());

        // BUGFIX (2026-06): klamra läspositionen till [1, bufLen-2]. Vid lång
        // eko-tid (TIME-knoben nära max ≈ 350 ms) ligger delaySmoothed redan på
        // bufLen-1; wow-LFO:ns positiva excursion (echoWowDepth·sin) drev då
        // fracDelay FÖRBI buffergränsen → delayInt > bufLen. Den gamla
        // enkelstegs-wrappen (if (r0<0) r0+=bufLen) kunde inte ta tillbaka
        // indexet i intervall → NEGATIVT index = OOB-läsning ur buf[-1..]
        // = skräp som återkom i takt med wow-frekvensen (~1,5 Hz) = periodiskt
        // digitalt sprak. jlimit håller läsningen innanför bufferten (samma
        // skydd WowFlutter redan har) och bevarar pitch-wandern under taket.
        const float fracDelay  = juce::jlimit (1.0f,
                                               static_cast<float> (bufLen) - 2.0f,
                                               delaySmoothed + wowOffset);
        const int   delayInt   = static_cast<int> (std::floor (fracDelay));
        const float delayFrac  = fracDelay - static_cast<float> (delayInt);

        // BUGFIX (2026-06): läspekaren MÅSTE avancera per sample. Partnerns
        // writeIdx är FRUSEN under hela detta blocket (partner-kanalen processas
        // vid en annan tidpunkt — L hela blocket, sedan R), så att indexera med
        // den gav en STATISK läsning → trappstegning vid varje blockgräns =
        // bredbandigt skräp ("distat oljud" vid SOS+echo). Använd EGEN writeIdx
        // (avancerar per sample); båda buffrarna skrivs i lockstep så de är i
        // synk vid blockgräns. Bevarar ping-pong-karaktären, tar bort skräpet.
        const int writeIdxForRead = writeIdx;
        // Robust modulo-wrap (full % istället för enkelsteg) — tål godtyckligt
        // delayInt och kan aldrig ge negativt/OOB-index.
        const int r0 = ((writeIdxForRead - delayInt    ) % bufLen + bufLen) % bufLen;
        const int r1 = ((writeIdxForRead - delayInt - 1) % bufLen + bufLen) % bufLen;
        const float delayed = readBuf[static_cast<size_t> (r0)] * (1.0f - delayFrac)
                            + readBuf[static_cast<size_t> (r1)] * delayFrac;

        // Separat AMOUNT (wet-nivå man hör) och FEEDBACK (regeneration/tail):
        //   output       = torr + amount   × delayed   (hur mycket echo man hör)
        //   recirkulation = torr + feedback × delayed   (hur länge svansen lever)
        // Soft-clip båda så varken record-kedjan eller delay-buffern ser > ~±1.5.
        // Self-osc (feedback > ~0.96) hålls bounded av tanh-clippet.
        constexpr float kClipKnee = 1.5f;
        const float y = kClipKnee * std::tanh ((x + delayed * amount) / kClipKnee);  // wet ut

        // Recirkulation: HF-loss per pass (band-eko tappar diskant) + soft-clip.
        float fbSignal = hfLossFilter.processSample (x + delayed * feedback);
        fbSignal = kClipKnee * std::tanh (fbSignal / kClipKnee);

        buf[static_cast<size_t> (writeIdx)] = fbSignal;
        writeIdx = (writeIdx + 1) % static_cast<int> (buf.size());

        return y;
    }

    void Echo::process (juce::AudioBuffer<float>& buffer, int channel)
    {
        if (! enabled || amount < 1e-6f) return;
        auto* data = buffer.getWritePointer (channel);
        const int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
            data[i] = processSample (data[i]);
    }
}
