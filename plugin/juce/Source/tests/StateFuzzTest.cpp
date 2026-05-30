/*  StateFuzzTest — robusthet mot TRASIG/MANIPULERAD plugin-state.

    StateRoundTripTest bevisar att KORREKT state round-trippar. Det här testet
    bevisar att FELAKTIG state inte kraschar pluginen eller producerar NaN-params
    / skenande output. En host (eller en korrupt projektfil, eller en illvillig
    preset) kan mata in vad som helst i setStateInformation — pluginen måste
    degradera säkert.

    Fuzzar:
      - tom / null / 0-byte
      - trunkerade prefix av giltig state
      - bit-flippad giltig state
      - ren slump-binär (olika storlekar)
      - GILTIG xml-struktur men förgiftade värden (NaN/inf/±1e30) i varje PARAM

    Efter varje setStateInformation verifieras: alla param-värden ändliga & i
    [0,1], och ett ljudblock processas ändligt och bounded (|x| ≤ 8). Körs även
    under ASan (BC2000DL_SANITIZE=ON) → minnessäkerhet på deserialiseringsvägen.

    Bygg:  cmake --build build --target BC2000DL_StateFuzzTest
    Exit:  0 = ingen krasch/NaN/blowup, 1 = avvikelse.
*/
#include "../PluginProcessor.h"
#include <cstdio>
#include <cmath>
#include <random>
#include <vector>

namespace
{
    int g_fails = 0;
    int g_cases = 0;

    bool paramsFinite (BC2000DLProcessor& p)
    {
        for (auto* prm : p.getParameters())
        {
            const float v = prm->getValue();         // normaliserat 0..1
            if (! std::isfinite (v) || v < -0.001f || v > 1.001f)
            {
                std::printf ("    -> icke-ändligt/utanför-range param-värde: %.5g\n", v);
                return false;
            }
        }
        return true;
    }

    bool processBounded (BC2000DLProcessor& p)
    {
        p.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer midi;
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = buf.getWritePointer (ch);
            for (int i = 0; i < 512; ++i)
                d[i] = 0.3f * std::sin (2.0 * 3.14159265 * 440.0 * i / 48000.0);
        }
        p.processBlock (buf, midi);
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            const auto* d = buf.getReadPointer (ch);
            for (int i = 0; i < 512; ++i)
                if (! std::isfinite (d[i]) || std::abs (d[i]) > 8.0f)
                {
                    std::printf ("    -> output ej bounded: %.5g\n", d[i]);
                    return false;
                }
        }
        return true;
    }

    // Mata in data → verifiera säker degradering.
    void feed (const void* data, int size, const char* label)
    {
        ++g_cases;
        BC2000DLProcessor p;
        p.setStateInformation (data, size);     // får ALDRIG krascha
        const bool ok = paramsFinite (p) && processBounded (p);
        if (! ok) { std::printf ("[FAIL] %s\n", label); ++g_fails; }
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    std::printf ("=== StateFuzzTest — robusthet mot trasig state ===\n");

    // Giltig referens-state (params satta till icke-default).
    juce::MemoryBlock valid;
    {
        BC2000DLProcessor a;
        int idx = 0;
        for (auto* prm : a.getParameters())
            prm->setValueNotifyingHost ((idx++ % 3 == 0) ? 0.2f : 0.7f);
        a.getStateInformation (valid);
    }
    std::printf ("Giltig state: %d bytes\n", (int) valid.getSize());

    std::mt19937 rng (0xBEEF2000u);

    // (1) tom / null
    feed (nullptr, 0, "null, size 0");
    { char b = 0; feed (&b, 0, "giltig ptr, size 0"); }

    // (2) trunkerade prefix
    for (int frac : { 1, 2, 4, 8, 16, 32 })
    {
        const int n = (int) valid.getSize() / frac;
        feed (valid.getData(), n, "trunkerad state-prefix");
    }

    // (3) bit-flippad giltig state (50 varianter)
    for (int it = 0; it < 50; ++it)
    {
        juce::MemoryBlock m (valid);
        auto* bytes = static_cast<uint8_t*> (m.getData());
        const int flips = 1 + (int) (rng() % 16);
        for (int f = 0; f < flips; ++f)
            bytes[rng() % m.getSize()] ^= (uint8_t) (1u << (rng() % 8));
        feed (m.getData(), (int) m.getSize(), "bit-flippad state");
    }

    // (4) ren slump-binär
    for (int sz : { 1, 4, 16, 64, 256, 1024, 4096 })
        for (int it = 0; it < 8; ++it)
        {
            std::vector<uint8_t> g ((size_t) sz);
            for (auto& b : g) b = (uint8_t) (rng() & 0xFF);
            feed (g.data(), sz, "slump-binär");
        }

    // (5) GILTIG xml-struktur, förgiftade VÄRDEN i varje PARAM. Byggs via xml-TEXT
    //     (setAttribute lagrar strängar) + manuell binär-wrap (JUCE:s format:
    //     magic 0x21324356 + längd + single-line xml) → exakt vad en korrupt
    //     projektfil innehåller, UTAN att gå via APVTS (som annars skulle synka
    //     in NaN i en live-param och trigga UB i SERIALISERINGEN under själva
    //     test-uppsättningen — det vi testar är DESERIALISERINGEN).
    juce::String validXml;
    {
        BC2000DLProcessor a;
        int idx = 0;
        for (auto* prm : a.getParameters())
            prm->setValueNotifyingHost ((idx++ % 3 == 0) ? 0.3f : 0.6f);
        validXml = a.apvts.copyState().toXmlString();
    }
    auto makeBinary = [] (const juce::XmlElement& xml)
    {
        juce::MemoryBlock mb;
        {
            juce::MemoryOutputStream out (mb, false);
            out.writeInt (0x21324356);                 // magicXmlNumber
            out.writeInt (0);                          // platshållare för längd
            xml.writeTo (out, juce::XmlElement::TextFormat().singleLine());
            out.writeByte (0);
        }
        static_cast<juce::uint32*> (mb.getData())[1] = (juce::uint32) (mb.getSize() - 9);
        return mb;
    };
    const char* strPoisons[] = {
        "nan", "NaN", "inf", "-inf", "1.#INF", "1e400", "-1e400", "garbage",
        "", "0x7ff8000000000000", "99999999999999999999", "1e30", "-1e30",
        "1e-30", "1234567", "true", "../../etc/passwd"
    };
    for (const char* bad : strPoisons)
    {
        auto xml = juce::parseXML (validXml);
        if (xml == nullptr) { std::printf ("[FAIL] kunde ej parsa giltig xml\n"); ++g_fails; break; }
        for (auto* e : xml->getChildIterator())
            if (e->hasTagName ("PARAM"))
                e->setAttribute ("value", juce::String (bad));
        auto mb = makeBinary (*xml);
        feed (mb.getData(), (int) mb.getSize(), "giltig xml, förgiftade param-värden");
    }

    std::printf ("\nKörde %d fall, %d underkända.\n", g_cases, g_fails);
    if (g_fails == 0)
        std::printf ("  OK — pluginen degraderar säkert vid all trasig state.\n");
    return g_fails == 0 ? 0 : 1;
}
