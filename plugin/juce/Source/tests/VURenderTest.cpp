/*  VURenderTest — definitivt RENDERINGS-test av WireframeVU-nålen.

    Renderar den riktiga WireframeVU-komponenten (samma klass som BEOLUX 2000
    DELUXE-editorn använder) offscreen till en juce::Image över ett dB-svep och
    detekterar nålens X-position per nivå.  Verifierar att:

      1. Nålen FAKTISKT ritas (rendering ≠ tom/idle).
      2. Nålen rör sig monotont åt höger när nivån ökar (-30 → -3 dBFS).
      3. Rörelsen är "synlig" (stor andel av meter-bredden) — fångar regressioner
         där datat matas in men nålen står still / inte syns.
      4. Samma sak håller vid den RIKTIGA pixel-storleken (50×22) som editorn
         lägger ut metrarna i — fångar buggar som bara märks vid liten storlek.

    Detta isolerar renderingssidan: datat (chain-atomics → setLevel) täcks redan
    av meter-data-testet; här bevisar vi att paint() faktiskt visualiserar nivån.

    Bygg:  cmake --build build --target BC2000DL_VURenderTest
    Exit:  0 = nålen renderar + rör sig korrekt, 1 = avvikelse.
*/
#include "../WireframeEditor.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <utility>

using bc2000dl::ui::WireframeVU;

namespace
{
    // Rita WireframeVU vid nivå dbfs (settle = kör ballistiken i botten).
    juce::Image renderVU (int w, int h, float dbfs, bool settle)
    {
        WireframeVU vu ("OUT L");
        vu.setBounds (0, 0, w, h);
        if (settle)
            for (int i = 0; i < 80; ++i)   // 80 ticks @ 0.18 coef → fullt settlat
                vu.setLevel (dbfs);

        juce::Image img (juce::Image::ARGB, w, h, true);
        juce::Graphics g (img);
        vu.paint (g);
        return img;
    }

    // Per-kolumn |brightness|-diff mot idle-bilden (nålen är enda rörliga elementet).
    std::vector<double> columnDiff (const juce::Image& idle, const juce::Image& lvl, int w, int h)
    {
        std::vector<double> d ((size_t) w, 0.0);
        for (int x = 0; x < w; ++x)
        {
            double s = 0.0;
            for (int y = 0; y < h; ++y)
                s += std::abs ((double) idle.getPixelAt (x, y).getBrightness()
                             - (double) lvl .getPixelAt (x, y).getBrightness());
            d[(size_t) x] = s;
        }
        return d;
    }

    // Nålens X = kolumnen med störst diff mot idle, BORTSETT från idle-nålens
    // hemvist längst till vänster (norm 0 → maskas bort).
    std::pair<int, double> needleX (const juce::Image& idle, int w, int h, float dbfs)
    {
        const auto lvl = renderVU (w, h, dbfs, true);
        const auto d   = columnDiff (idle, lvl, w, h);
        const int  x0  = (int) (w * 0.15);   // maska idle-nålens far-left-zon
        int    best = x0;
        double bv   = -1.0;
        for (int x = x0; x < w; ++x)
            if (d[(size_t) x] > bv) { bv = d[(size_t) x]; best = x; }
        return { best, bv };
    }

    int g_fails = 0;
    void check (bool cond, const char* msg)
    {
        std::printf ("  [%s] %s\n", cond ? "OK  " : "FAIL", msg);
        if (! cond) ++g_fails;
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::printf ("=== VURenderTest — WireframeVU nål-rendering ===\n");

    // ---- (A) Hög-upplöst svep (240×88) för robust detektion ----
    {
        const int W = 240, H = 88;
        const auto idle = renderVU (W, H, -60.0f, false);

        const auto n30 = needleX (idle, W, H, -30.0f);
        const auto n20 = needleX (idle, W, H, -20.0f);
        const auto n10 = needleX (idle, W, H, -10.0f);
        const auto n03 = needleX (idle, W, H,  -3.0f);

        std::printf ("\n[240x88]  needleX:  -30=%d  -20=%d  -10=%d  -3=%d  (W=%d)\n",
                     n30.first, n20.first, n10.first, n03.first, W);

        // Nålen ritas faktiskt (diff-energi över brus-tröskel vid varje nivå)
        check (n30.second > 1.0 && n03.second > 1.0,
               "nalen renderas (diff-energi > troskel)");

        // Monoton ökning åt höger med stigande nivå
        check (n30.first < n20.first && n20.first < n10.first && n10.first < n03.first,
               "nalen ror sig monotont hoger -30<-20<-10<-3");

        // Synlig rörelse: -30 → -3 ska täcka stor del av bredden
        check ((n03.first - n30.first) > (int) (W * 0.40),
               "rorelse -30..-3 > 40% av bredden (synlig)");

        // -3 dBFS hamnar i röd-zonen (norm > 0.80 → x bortom 75% av bredden)
        check (n03.first > (int) (W * 0.70),
               "-3 dBFS landar langt hoger (rod-zon)");
    }

    // ---- (B) RIKTIG editor-storlek (50×22) — fångar small-size-buggar ----
    {
        const int W = 50, H = 22;
        const auto idle = renderVU (W, H, -60.0f, false);

        const auto n30 = needleX (idle, W, H, -30.0f);
        const auto n03 = needleX (idle, W, H,  -3.0f);

        std::printf ("\n[50x22]   needleX:  -30=%d  -3=%d  (W=%d)\n",
                     n30.first, n03.first, W);

        check (n03.second > 0.5, "nalen renderas aven vid 50x22");
        check (n03.first > n30.first, "nalen ror sig hoger aven vid 50x22");
        check ((n03.first - n30.first) >= 8, "rorelse >= 8px vid 50x22 (synlig)");
    }

    std::printf ("\n");
    if (g_fails == 0)
        std::printf ("RESULTAT: VU-nålen renderar och spårar nivå korrekt — OK\n");
    else
        std::printf ("RESULTAT: %d kontroll(er) underkända\n", g_fails);

    return g_fails == 0 ? 0 : 1;
}
