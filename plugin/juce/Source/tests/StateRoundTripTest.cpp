/*  StateRoundTripTest — definitivt test av plugin-state-serialisering.

    Konstruerar BC2000DLProcessor, sätter params till icke-default, kör
    getStateInformation → setStateInformation på en FÄRSK processor och
    verifierar att ALLA params round-trippar (inkl. de nya: master_volume_r,
    input_trim, output_trim, mains_hum, echo_*, bias_amount_r, mic_mode ...).

    Direkt mot processorn (ingen pedalboard) → testar den riktiga
    getStateInformation/setStateInformation-koden.

    Bygg:  cmake --build build --target BC2000DL_StateTest
    Exit:  0 = alla params round-trippar, 1 = avvikelse.
*/
#include "../PluginProcessor.h"
#include <cstdio>
#include <cmath>

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;   // message manager för AudioProcessor

    BC2000DLProcessor a;

    // Sätt VARJE param till ett icke-default normaliserat värde (deterministiskt).
    auto& params = a.getParameters();
    int idx = 0;
    for (auto* p : params)
    {
        // växla mellan 0.2/0.5/0.8 så alla skiljer sig från default på olika sätt
        const float norm = (idx % 3 == 0) ? 0.2f : (idx % 3 == 1) ? 0.8f : 0.45f;
        p->setValueNotifyingHost (norm);
        ++idx;
    }

    juce::MemoryBlock mb;
    a.getStateInformation (mb);

    BC2000DLProcessor b;
    b.setStateInformation (mb.getData(), (int) mb.getSize());

    // Jämför FAKTISKA (denormaliserade) värden per param-ID — snappat för bools,
    // riktig enhet för floats/choices. (getValue() normaliserat snappar ej bools.)
    int fails = 0, total = 0;
    for (auto child : a.apvts.state)
    {
        if (! child.hasType ("PARAM")) continue;
        const auto id = child["id"].toString();
        auto* ra = a.apvts.getRawParameterValue (id);
        auto* rb = b.apvts.getRawParameterValue (id);
        if (ra == nullptr || rb == nullptr) continue;
        ++total;
        const float va = ra->load(), vb = rb->load();
        if (std::abs (va - vb) > 1.0e-3f)
        {
            std::printf ("[FAIL] %-22s  a=%.4f  b=%.4f\n", id.toRawUTF8(), va, vb);
            ++fails;
        }
    }

    std::printf ("\nState round-trip: %d/%d params round-trippade korrekt\n",
                 total - fails, total);
    if (fails == 0)
        std::printf ("  OK — getStateInformation/setStateInformation round-trippar allt.\n");
    return fails == 0 ? 0 : 1;
}
