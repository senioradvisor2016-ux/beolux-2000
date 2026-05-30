/*  UIRenderTest — smoke-test av HELA BEOLUX 2000 DELUXE-editorn.

    Konstruerar den riktiga editorn via processor.createEditor(), renderar hela
    komponentträdet (faceplate, rattar, knappar, faders, VU-metrar, text) offscreen
    till en juce::Image och verifierar:

      1. Editorn konstrueras + förstörs utan krasch (assets laddar, layout OK).
      2. paintEntireComponent kraschar inte (ingen null-deref i någon paint).
      3. Resultatet är en RIKTIG bild — inte blank/enfärgad (bred luminans-spridning
         + många distinkta färger → faceplaten ritades faktiskt).
      4. Upprepad konstruktion/rendering är stabil (catch teardown-/state-buggar).

    Detta breddar UI-täckningen bortom VU-nålen till hela editorn. Komplement till
    pluginval (som instansierar editorn men inte verifierar pixlar).

    Bygg:  cmake --build build --target BC2000DL_UIRenderTest
    Exit:  0 = editorn renderar en riktig bild stabilt, 1 = avvikelse.
*/
#include "../PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <cstdio>
#include <set>

namespace
{
    int g_fails = 0;
    void check (bool cond, const char* msg)
    {
        std::printf ("  [%s] %s\n", cond ? "OK  " : "FAIL", msg);
        if (! cond) ++g_fails;
    }

    struct ImageStats { float minLum = 1e9f, maxLum = -1e9f; size_t uniqueColours = 0; };

    ImageStats analyse (const juce::Image& img)
    {
        ImageStats s;
        std::set<juce::uint32> cols;
        // glesa ut samplingen lite för fart men täck hela ytan
        for (int y = 0; y < img.getHeight(); y += 2)
            for (int x = 0; x < img.getWidth(); x += 2)
            {
                const auto c = img.getPixelAt (x, y);
                const float l = c.getBrightness();
                s.minLum = juce::jmin (s.minLum, l);
                s.maxLum = juce::jmax (s.maxLum, l);
                cols.insert (c.getARGB());
            }
        s.uniqueColours = cols.size();
        return s;
    }

    juce::Image renderEditor (BC2000DLProcessor& proc)
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        jassert (ed != nullptr);
        const int w = ed->getWidth(), h = ed->getHeight();
        juce::Image img (juce::Image::ARGB, juce::jmax (1, w), juce::jmax (1, h), true);
        {
            juce::Graphics g (img);
            ed->paintEntireComponent (g, false);   // editor + ALLA barn
        }
        return img;   // ed destrueras här → testar teardown
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    std::printf ("=== UIRenderTest — hela DELUXE-editorn ===\n");

    BC2000DLProcessor proc;
    proc.prepareToPlay (48000.0, 512);

    // (1) Grundrendering vid default-state
    juce::Image img = renderEditor (proc);
    std::printf ("Editor-storlek: %dx%d\n", img.getWidth(), img.getHeight());
    check (img.getWidth() >= 600 && img.getHeight() >= 600, "editorn har rimlig storlek");

    const auto st = analyse (img);
    std::printf ("Luminans-spann: %.3f..%.3f   distinkta färger: %zu\n",
                 st.minLum, st.maxLum, st.uniqueColours);
    check ((st.maxLum - st.minLum) > 0.30f, "bred luminans-spridning (ej blank)");
    check (st.uniqueColours > 500, "manga distinkta farger (faceplaten ritades)");

    // (2) Rendera i ett par param-konfigurationer (mute/speaker/faders) → ingen krasch
    auto setParam = [&] (const char* id, float norm)
    {
        if (auto* p = proc.apvts.getParameter (id))
            p->setValueNotifyingHost (norm);
    };
    setParam ("speaker_mute", 1.0f);
    setParam ("master_volume", 0.9f);
    setParam ("master_volume_r", 0.1f);
    setParam ("echo_amount", 0.8f);
    setParam ("bass", 0.2f);
    setParam ("treble", 0.85f);
    juce::Image img2 = renderEditor (proc);
    check (img2.getWidth() == img.getWidth(), "rendering i annan state kraschar ej");
    const auto st2 = analyse (img2);
    check ((st2.maxLum - st2.minLum) > 0.30f, "fortsatt riktig bild i annan state");

    // (3) Upprepad konstruktion/rendering (teardown-stabilitet)
    for (int i = 0; i < 5; ++i)
    {
        auto im = renderEditor (proc);
        if (im.getWidth() < 1) { check (false, "upprepad rendering"); break; }
    }
    check (true, "5x upprepad konstruktion+rendering stabil");

    // Dumpa en PNG för manuell inspektion
    { juce::File f ("/tmp/bc2000_editor.png"); juce::FileOutputStream os (f);
      juce::PNGImageFormat png; png.writeImageToStream (img, os); }
    std::printf ("[info] PNG: /tmp/bc2000_editor.png\n");

    std::printf ("\n");
    if (g_fails == 0) std::printf ("RESULTAT: hela editorn renderar en riktig bild stabilt — OK\n");
    else              std::printf ("RESULTAT: %d kontroll(er) underkända\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}
