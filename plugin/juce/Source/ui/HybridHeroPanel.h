/*  HybridHeroPanel — bakgrund för plugin-editor.

    Ritar:
    - Teak-trä-ram (yttre)
    - Svart anodiserad alu-panel (inner)
    - Brushed silver tape-deck-yta överst (med 2 spolar + tape-head-cover)
    - Logo "BC2000DL · DANISH TAPE 2000" i embossed-stil

    Är en ren paint()-component — inga interaktiva element.

    Plats: plugin/juce/Source/ui/HybridHeroPanel.h
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BCColours.h"
#include "PhotorealReel.h"

namespace bc2000dl::ui
{
    class HybridHeroPanel : public juce::Component, private juce::Timer
    {
    public:
        HybridHeroPanel();
        void paint (juce::Graphics&) override;

        /** Aktivera animerad spol-rotation (när transport är PLAY). */
        void setReelsRotating (bool r) { reelsRotating = r; }

        /** Reservera utrymme i topp för header-bar (ritas inte av panelen). */
        void setHeaderHeight (int px)  { headerHeight = px; repaint(); }

        /** Reservera utrymme i botten för status-bar (ritas inte av panelen). */
        void setFooterHeight (int px)  { footerHeight = px; repaint(); }

        /** Sektionsgränser (x-koordinater i panelen) för att rita avskiljare. */
        void setSectionDividers (int leftX, int rightX) { dividerLeftX = leftX; dividerRightX = rightX; repaint(); }

    private:
        void timerCallback() override;
        void ensureReelImagesPrepared (int sizePx);

        bool reelsRotating { false };
        float reelAngle { 0.0f };
        int   headerHeight { 0 };
        int   footerHeight { 0 };
        int   dividerLeftX { 0 };
        int   dividerRightX { 0 };

        // Pre-renderade high-quality reel-images (cached vid sizePx)
        juce::Image reelImageLeft, reelImageRight;
        int cachedReelSizePx { 0 };
    };
}
