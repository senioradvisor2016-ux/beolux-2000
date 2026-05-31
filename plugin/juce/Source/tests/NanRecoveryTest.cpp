/*  NanRecoveryTest — HOSTLÖS verifiering av NaN-självläkning.

    Kör SignalChain DIREKT (ingen host, inget pedalboard) så de rekursiva
    tillstånden bevaras EXAKT mellan block — precis som i Ableton/Logic. Det är
    avgörande: pedalboard/pluginval sanerar/återställer själva vid NaN-output och
    DÖLJER därför om pluginen latchar. En riktig DAW gör det inte.

    Matematiskt faktum: när ett IIR-filters state blir NaN ger det NaN för ALLT
    framtida block oavsett insignal — om det inte återställs. Pluginen har många
    IIR-steg (DC-block, playEq, ton, RIAA, shelves) + echo-feedback + Jiles-
    Atherton-tape. Utan NaN-vakten latchar en enda NaN → permanent tystnad
    (= "Ableton tystar appen efter ett tag").

    Testet: värm upp → injicera NaN/Inf i ett block → kör rena block och
    verifiera att output blir ÄNDLIG + icke-tyst igen (självläkning).

    Bygg:  cmake --build build --target BC2000DL_NanTest
    Exit:  0 = återhämtar sig, 1 = latchade (permanent NaN/tyst).
*/
#include "../dsp/SignalChain.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cstdio>
#include <cmath>
#include <limits>

using namespace bc2000dl::dsp;

static int g_fails = 0;

// Kör värm-upp → brutal NaN/Inf-injektion → ren signal, och verifiera att
// pluginen ALDRIG släpper ut NaN/Inf (host:en, t.ex. Ableton, deaktiverar
// spåret permanent vid en enda NaN) OCH att signalen kommer tillbaka.
static void runMode (const char* label, bool bypassTape, int monitorMode)
{
    const double sr = 48000.0;
    const int    n  = 512;

    SignalChain chain;
    chain.prepare (sr, n);

    SignalChain::Parameters p;
    p.micGain = 0.7f; p.micGainR = 0.7f;
    p.masterVolume = 0.85f; p.masterVolumeR = 0.85f;
    p.monitorMode = monitorMode;
    p.bypassTape  = bypassTape;
    p.echoEnabled = true;
    p.echoAmount = 0.8f; p.echoAmountR = 0.8f;
    p.echoFeedback = 0.9f;

    juce::AudioBuffer<float> buf (2, n);
    double ph = 0.0;
    const double dphi = 2.0 * juce::MathConstants<double>::pi * 220.0 / sr;
    auto fillSine = [&]
    {
        for (int i = 0; i < n; ++i)
        { const float s = 0.4f * (float) std::sin (ph); ph += dphi;
          buf.setSample (0, i, s); buf.setSample (1, i, s); }
    };
    auto blockFinite = [&] (float& peakOut)
    {
        bool finite = true; float pk = 0.0f;
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            { const float v = buf.getSample (c, i);
              if (! std::isfinite (v)) finite = false;
              pk = std::max (pk, std::abs (v)); }
        peakOut = pk; return finite;
    };

    for (int b = 0; b < 20; ++b) { fillSine(); chain.setParameters (p); chain.process (buf); }

    int badBlocks = 0;
    for (int b = 0; b < 3; ++b)   // 3 hela NaN/Inf-block i rad
    {
        const float bad = (b == 1) ? std::numeric_limits<float>::infinity()
                                   : std::numeric_limits<float>::quiet_NaN();
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i) buf.setSample (c, i, bad);
        chain.setParameters (p); chain.process (buf);
        float pk = 0.0f; if (! blockFinite (pk)) ++badBlocks;
    }

    int recoverBlock = -1;
    for (int b = 0; b < 40; ++b)
    {
        fillSine(); chain.setParameters (p); chain.process (buf);
        float pk = 0.0f; const bool fin = blockFinite (pk);
        if (! fin) ++badBlocks;
        if (fin && pk > 1e-4f && recoverBlock < 0) recoverBlock = b;
    }

    const bool ok = (badBlocks == 0 && recoverBlock >= 0);
    std::printf ("  [%s] %-26s  läckta-NaN-block=%d  recover=block %d\n",
                 ok ? "OK  " : "FAIL", label, badBlocks, recoverBlock);
    if (! ok) ++g_fails;
}

int main()
{
    std::printf ("=== NanRecoveryTest — ingen NaN/Inf får lämna pluginen ===\n");
    runMode ("Tape (tape skrubbar)",   false, 1);   // tape-vägen NaN-scrubbar redan
    runMode ("Source (förbi tape)",    false, 0);   // monitor=Source → FÖRBI tape-scrub
    runMode ("Bypass-tape (förbi)",    true,  1);   // bypass → FÖRBI tape-scrub
    if (g_fails == 0)
        std::printf ("RESULTAT: ingen NaN lämnar pluginen i NÅGON väg — host:en mutar aldrig spåret. OK\n");
    else
        std::printf ("RESULTAT: %d väg(ar) läckte NaN → host hade mutat spåret\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}
