/*  CpuBenchTest — realtidsfaktor-regression för hela DSP-kedjan.

    UAD-nivå betyder också "20 instanser i en mix utan att fläkten ryter".
    Testet kör SignalChain (stereo, 48 kHz, 512-block, fullt påslagen kedja
    inkl. echo + wow + print-through) över N sekunder programmaterial och
    mäter wall-clock → realtidsfaktor (RT× = audio-sekunder per CPU-sekund).

    Gräns: konservativ (≥ 5×) så CI-runners med delade kärnor inte flakear —
    en Apple-M-maskin ligger typiskt långt över. Syftet är att fånga
    REGRESSIONER (en Fas-2-ändring som halverar throughput syns direkt),
    inte att benchmarka exakt.

    Bygg:  cmake --build build --target BC2000DL_CpuBench
    Exit:  0 = över gränsen, 1 = under (regression).
*/
#include "../dsp/SignalChain.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <chrono>
#include <cstdio>
#include <cmath>

using namespace bc2000dl::dsp;

int main()
{
    constexpr double sr = 48000.0;
    constexpr int blockSize = 512;
    constexpr double seconds = 20.0;
    constexpr double minRtFactor = 5.0;

    SignalChain chain;
    chain.prepare (sr, blockSize);

    SignalChain::Parameters p;
    p.micGain = p.micGainR = 0.6f;
    p.phonoGain = p.phonoGainR = 0.3f;     // aktivera phono-bussen också
    p.wowFlutterAmount = 1.0f;
    p.echoEnabled = true;
    p.echoAmount = p.echoAmountR = 0.4f;
    p.printThrough = 0.02f;
    p.masterVolume = p.masterVolumeR = 0.8f;
    chain.setParameters (p);

    juce::AudioBuffer<float> buf (2, blockSize);
    const int numBlocks = static_cast<int> (seconds * sr / blockSize);

    // Programmaterial: blandade toner + brus (LCG), deterministiskt
    std::uint32_t seed = 0x5EED1968u;
    float ph1 = 0.0f, ph2 = 0.0f;

    // Värm upp (cache/branch predictors + DSP-state) utan att mäta
    for (int b = 0; b < 50; ++b)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = buf.getWritePointer (ch);
            for (int i = 0; i < blockSize; ++i) d[i] = 0.1f;
        }
        chain.process (buf);
    }

    const auto t0 = std::chrono::steady_clock::now();
    for (int b = 0; b < numBlocks; ++b)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = buf.getWritePointer (ch);
            for (int i = 0; i < blockSize; ++i)
            {
                seed = seed * 1664525u + 1013904223u;
                const float ns = static_cast<float> (static_cast<std::int32_t> (seed))
                               * (1.0f / 2147483648.0f);
                d[i] = 0.15f * std::sin (ph1) + 0.08f * std::sin (ph2) + 0.02f * ns;
            }
        }
        ph1 += 0.0524f * blockSize;  ph1 = std::fmod (ph1, 6.2832f);
        ph2 += 0.3142f * blockSize;  ph2 = std::fmod (ph2, 6.2832f);
        chain.process (buf);
    }
    const auto t1 = std::chrono::steady_clock::now();

    const double cpuSec = std::chrono::duration<double> (t1 - t0).count();
    const double audioSec = numBlocks * blockSize / sr;
    const double rt = audioSec / cpuSec;

    std::printf ("CPU-bench: %.1f s audio på %.3f s CPU → RT-faktor %.1fx "
                 "(graens %.1fx)\n", audioSec, cpuSec, rt, minRtFactor);

    if (rt < minRtFactor)
    {
        std::printf ("FAIL: under regressionsgraensen\n");
        return 1;
    }
    std::printf ("CPU-BENCH OK\n");
    return 0;
}
