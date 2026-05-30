/*  HostRenderTest — headless REAL-HOST render via JUCE:s VST3-värd.

    Laddar den FÄRDIGBYGGDA .vst3-binären (inte SharedCode-symbolerna) genom
    JUCE:s VST3PluginFormat — exakt det host-lager som pluginval och kommersiella
    DAW:er använder för att scanna + instansiera plugins. Renderar sedan en ton
    GENOM pluggen och verifierar:

      1. Värden hittar + instansierar VST3:n från disk (format-nivå, som en DAW).
      2. Pluggen rapporterar rimlig latency/bus-layout och preparerar utan fel.
      3. Utsignalen är icke-tyst, bounded (|x| ≤ 8) och fri från NaN/inf.
      4. Pluggen FÄRGAR signalen (ut ≠ in-ton) — bevisar verklig FX-processning
         i en host (det REAPER:s pre-FX-accessor inte kunde visa).

    Helt headless + deterministiskt → körs i ctest (integration). Kräver att
    VST3:n är installerad (COPY_PLUGIN_AFTER_BUILD lägger den i user-VST3).

    Bygg:  cmake --build build --target BC2000DL_HostRenderTest
    Exit:  0 = laddas + processar korrekt via VST3-värd, 1 = avvikelse.
*/
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <cstdio>
#include <cmath>

namespace
{
    int g_fails = 0;
    void check (bool c, const char* m)
    {
        std::printf ("  [%s] %s\n", c ? "OK  " : "FAIL", m);
        if (! c) ++g_fails;
    }

    juce::File findInstalledVST3()
    {
        const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
        const juce::File cands[] = {
            home.getChildFile ("Library/Audio/Plug-Ins/VST3/Beolux 2000.vst3"),
            juce::File ("/Library/Audio/Plug-Ins/VST3/Beolux 2000.vst3")
        };
        for (const auto& f : cands) if (f.exists()) return f;
        return {};
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    std::printf ("=== HostRenderTest — VST3-värd (JUCE) renderar ton genom pluggen ===\n");

    const auto vst3 = findInstalledVST3();
    if (! vst3.exists())
    {
        std::printf ("[FAIL] hittar ej installerad VST3 (kör bygget först).\n");
        return 1;
    }
    std::printf ("VST3: %s\n", vst3.getFullPathName().toRawUTF8());

    // --- (1) Scanna + instansiera via VST3-formatet (som en DAW) ---
    juce::VST3PluginFormat fmt;
    juce::OwnedArray<juce::PluginDescription> types;
    fmt.findAllTypesForFile (types, vst3.getFullPathName());
    check (types.size() > 0, "VST3-värden hittar plugin-typ i binären");
    if (types.isEmpty()) { std::printf ("RESULTAT: kunde ej scanna VST3\n"); return 1; }

    std::printf ("Namn: %s  | tillverkare: %s  | format: %s\n",
                 types[0]->name.toRawUTF8(), types[0]->manufacturerName.toRawUTF8(),
                 types[0]->pluginFormatName.toRawUTF8());

    const double sr = 48000.0;
    const int    block = 512;
    juce::AudioPluginFormatManager mgr;
    mgr.addDefaultFormats();
    juce::String err;
    std::unique_ptr<juce::AudioPluginInstance> inst (
        mgr.createPluginInstance (*types[0], sr, block, err));
    check (inst != nullptr, "VST3:n instansieras från disk via värden");
    if (inst == nullptr) { std::printf ("  fel: %s\n", err.toRawUTF8()); return 1; }

    // --- (2) Prepara + grundläggande host-egenskaper ---
    inst->enableAllBuses();
    inst->setRateAndBufferSizeDetails (sr, block);
    inst->prepareToPlay (sr, block);
    const int latency = inst->getLatencySamples();
    const int nIn  = inst->getTotalNumInputChannels();
    const int nOut = inst->getTotalNumOutputChannels();
    std::printf ("Kanaler in/ut: %d/%d   latency: %d samples\n", nIn, nOut, latency);
    check (nOut >= 2, "pluggen exponerar stereo-ut i värden");
    check (latency >= 0 && latency < (int) sr, "latency-rapport rimlig (0..1s)");

    // --- (3+4) Rendera ~1s 440 Hz-ton genom pluggen, mät in vs ut ---
    const int chans  = juce::jmax (2, juce::jmax (nIn, nOut));
    const int blocks = (int) (sr / block);   // ~1 sek
    double inSumSq = 0, outSumSq = 0, outPeak = 0, maxDiff = 0;
    int nan = 0; long samples = 0;
    double phase = 0.0;
    const double dphi = 2.0 * juce::MathConstants<double>::pi * 440.0 / sr;

    juce::AudioBuffer<float> buf (chans, block);
    juce::MidiBuffer midi;
    for (int b = 0; b < blocks; ++b)
    {
        buf.clear();
        for (int i = 0; i < block; ++i)
        {
            const float s = 0.3f * (float) std::sin (phase); phase += dphi;
            for (int c = 0; c < juce::jmax (1, nIn); ++c) buf.setSample (c, i, s);
            inSumSq += (double) s * s;   // referens-in (mono-ekvivalent)
        }
        inst->processBlock (buf, midi);
        for (int c = 0; c < nOut; ++c)
            for (int i = 0; i < block; ++i)
            {
                const float v = buf.getSample (c, i);
                if (! std::isfinite (v)) ++nan;
                const double a = std::abs ((double) v);
                if (a > outPeak) outPeak = a;
                outSumSq += (double) v * v;
                const double d = std::abs ((double) v - 0.3 * std::sin (phase - dphi * (block - i)));
                if (d > maxDiff) maxDiff = d;
            }
        samples += block;
    }
    const double inRMS  = std::sqrt (inSumSq / samples);
    const double outRMS = std::sqrt (outSumSq / (samples * juce::jmax (1, nOut)));
    std::printf ("in-RMS %.5f   ut-RMS %.5f   ut-peak %.5f   NaN %d\n",
                 inRMS, outRMS, outPeak, nan);

    check (nan == 0, "ingen NaN/inf i värdens utsignal");
    check (outPeak < 8.0, "utsignal bounded (peak < 8)");
    check (outRMS > 0.0001, "icke-tyst utsignal genom värden");
    check (std::abs (outRMS - inRMS) > 0.001 || maxDiff > 0.01,
           "pluggen FÄRGAR signalen i värden (ut != in-ton)");

    inst->releaseResources();
    inst.reset();

    std::printf ("\n");
    if (g_fails == 0)
        std::printf ("RESULTAT: pluggen laddas + processar korrekt via VST3-värd — OK\n");
    else
        std::printf ("RESULTAT: %d kontroll(er) underkända\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}
