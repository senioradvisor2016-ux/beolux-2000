/*  PresetBrowser — implementation. */

#include "PresetBrowser.h"
#include "BCColours.h"
#include <cstring>

namespace bc2000dl::ui
{
    using namespace juce;
    using namespace bc2000dl::ui::colours;

    // ----------------------------------------------------------------
    // Fonts — keep them local so we don't pull in the full L&F header
    // ----------------------------------------------------------------
    static Font tabFont()  { return Font (FontOptions {}.withHeight (11.0f).withStyle ("Bold")); }
    static Font nameFont() { return Font (FontOptions {}.withHeight (12.0f).withStyle ("Bold")); }
    static Font descFont() { return Font (FontOptions {}.withHeight (10.5f)); }
    static Font pillFont() { return Font (FontOptions {}.withHeight  (9.0f).withStyle ("Bold")); }

    // ----------------------------------------------------------------
    // Tape-formula and speed pills
    // ----------------------------------------------------------------
    static const char* formulaLabel (int f)
    {
        switch (f) { case 1: return "BASF"; case 2: return "SCOTCH"; default: return "AGFA"; }
    }
    static const char* speedLabel (int s)
    {
        switch (s) { case 0: return "4.75"; case 1: return "9.5"; default: return "19"; }
    }
    static Colour formulaColour (int f)
    {
        // Agfa=amber, BASF=blue-teal, Scotch=warm green
        switch (f)
        {
            case 1:  return Colour (0xff2a5a6a);  // BASF teal
            case 2:  return Colour (0xff2a4a2a);  // Scotch green
            default: return Colour (0xff5a3a10);  // Agfa amber-brown
        }
    }

    // ----------------------------------------------------------------
    PresetBrowser::PresetBrowser()
    {
        setInterceptsMouseClicks (true, true);
        setWantsKeyboardFocus (true);
        rebuildFilteredList();
    }


    // ----------------------------------------------------------------
    void PresetBrowser::openBrowser (int currentPresetIndex)
    {
        currentPreset = currentPresetIndex;
        selectedCat = 0;               // reset to ALL
        scrollOffset = 0.0f;
        hoverRow = -1;
        rebuildFilteredList();

        // Scroll so the current preset is visible
        const int row = filteredIndices.indexOf (currentPreset);
        if (row >= 0)
        {
            const auto la = listArea();
            const float rowY = (float) row * kRowH - scrollOffset;
            if (rowY < 0.0f)
                scrollOffset = (float) row * kRowH;
            else if (rowY + kRowH > (float) la.getHeight())
                scrollOffset = (float) row * kRowH - (float) la.getHeight() + kRowH;
            clampScroll();
        }

        buildTabRects();
        setVisible (true);
        toFront (false);
        grabKeyboardFocus();
        repaint();
    }

    // ----------------------------------------------------------------
    void PresetBrowser::rebuildFilteredList()
    {
        filteredIndices.clear();
        for (int i = 0; i < kNumPresets; ++i)
        {
            const bool inCat =
                (selectedCat == 0) ||
                (std::strcmp (kPresets[i].category, kCategories[selectedCat]) == 0);
            if (inCat)
                filteredIndices.add (i);
        }
    }

    // ----------------------------------------------------------------
    Rectangle<int> PresetBrowser::listArea() const
    {
        return getLocalBounds().withTrimmedTop (kTabH + 1);
    }

    int PresetBrowser::rowAtY (int y) const
    {
        const auto la = listArea();
        const int relY = y - la.getY() + (int) scrollOffset;
        if (relY < 0) return -1;
        const int row = relY / kRowH;
        if (row < 0 || row >= filteredIndices.size()) return -1;
        return row;
    }

    // ----------------------------------------------------------------
    float PresetBrowser::maxScroll() const
    {
        const float totalH = (float) filteredIndices.size() * kRowH;
        const float viewH  = (float) listArea().getHeight();
        return jmax (0.0f, totalH - viewH);
    }

    void PresetBrowser::clampScroll()
    {
        scrollOffset = jlimit (0.0f, maxScroll(), scrollOffset);
    }

    // ----------------------------------------------------------------
    void PresetBrowser::buildTabRects()
    {
        tabRects.clear();
        const int tabY = 0;
        const int tabAreaW = getWidth();

        // Measure each tab label
        const auto f = tabFont();
        int x = 12;
        for (int i = 0; i < kNumCategories; ++i)
        {
            const String lbl (kCategories[i]);
            const int tw = (int) GlyphArrangement::getStringWidthInt (f, lbl) + 24;
            tabRects.add ({ i, { x, tabY + 3, tw, kTabH - 6 } });
            x += tw + 4;
            if (x > tabAreaW - 20) break;  // don't overflow
        }
    }

    int PresetBrowser::tabAtPoint (Point<int> p) const
    {
        for (const auto& t : tabRects)
            if (t.r.contains (p)) return t.catIdx;
        return -1;
    }

    // ----------------------------------------------------------------
    void PresetBrowser::selectCategory (int idx)
    {
        if (selectedCat == idx) return;
        selectedCat = idx;
        scrollOffset = 0.0f;
        hoverRow = -1;
        rebuildFilteredList();
        buildTabRects();
        repaint();
    }

    // ----------------------------------------------------------------
    void PresetBrowser::mouseDown (const MouseEvent& e)
    {
        const auto pt = e.getPosition();

        // Tab bar?
        const int tab = tabAtPoint (pt);
        if (tab >= 0)
        {
            selectCategory (tab);
            return;
        }

        // Preset row?
        const auto la = listArea();
        if (la.contains (pt))
        {
            const int row = rowAtY (pt.y);
            if (row >= 0 && row < filteredIndices.size())
            {
                const int presetIdx = filteredIndices[row];
                currentPreset = presetIdx;
                if (onPresetSelected) onPresetSelected (presetIdx);
                setVisible (false);
                return;
            }
        }

        // Click outside → close
        setVisible (false);
    }

    void PresetBrowser::mouseMove (const MouseEvent& e)
    {
        const int newHover = rowAtY (e.y);
        if (newHover != hoverRow)
        {
            hoverRow = newHover;
            repaint (listArea());
        }
    }

    void PresetBrowser::mouseExit (const MouseEvent&)
    {
        if (hoverRow != -1) { hoverRow = -1; repaint (listArea()); }
    }

    void PresetBrowser::mouseWheelMove (const MouseEvent&,
                                        const MouseWheelDetails& w)
    {
        scrollOffset -= w.deltaY * 60.0f;
        clampScroll();
        repaint (listArea());
    }

    // ----------------------------------------------------------------
    bool PresetBrowser::keyPressed (const KeyPress& k)
    {
        if (k == KeyPress::escapeKey) { setVisible (false); return true; }

        if (k == KeyPress::upKey || k == KeyPress::downKey)
        {
            const int cur = filteredIndices.indexOf (currentPreset);
            const int next = jlimit (0, filteredIndices.size() - 1,
                                     cur + (k == KeyPress::downKey ? 1 : -1));
            if (next != cur)
            {
                currentPreset = filteredIndices[next];
                if (onPresetSelected) onPresetSelected (currentPreset);
                // Keep it in view
                const auto la = listArea();
                const float rowY = (float) next * kRowH - scrollOffset;
                if (rowY < 0) scrollOffset = (float) next * kRowH;
                else if (rowY + kRowH > (float) la.getHeight())
                    scrollOffset = (float) next * kRowH - (float) la.getHeight() + kRowH;
                clampScroll();
                repaint();
            }
            return true;
        }

        if (k == KeyPress::returnKey)
        {
            setVisible (false);
            return true;
        }
        return false;
    }

    // ----------------------------------------------------------------
    void PresetBrowser::resized()
    {
        buildTabRects();
    }

    // ----------------------------------------------------------------
    void PresetBrowser::drawPill (Graphics& g,
                                   Rectangle<int>& cursor,
                                   const String& label,
                                   Colour bg, Colour fg) const
    {
        const int pw = GlyphArrangement::getStringWidthInt (pillFont(), label) + 10;
        const auto pr = cursor.removeFromLeft (pw).reduced (0, 7);
        cursor.removeFromLeft (4);
        g.setColour (bg);
        g.fillRoundedRectangle (pr.toFloat(), 3.0f);
        g.setColour (fg);
        g.setFont (pillFont());
        g.drawText (label, pr, Justification::centred);
    }

    // ----------------------------------------------------------------
    void PresetBrowser::drawRow (Graphics& g,
                                  Rectangle<int> rowR,
                                  int presetIdx,
                                  bool isSelected,
                                  bool isHovered) const
    {
        const auto& p = kPresets[presetIdx];

        // Row background
        if (isSelected)
            g.setColour (kAmberDim.withAlpha (0.7f));
        else if (isHovered)
            g.setColour (Colour (0xff1e1a14));
        else
            g.setColour (Colour (0xff141008));
        g.fillRect (rowR);

        // Thin separator
        g.setColour (Colour (0xff2a2418));
        g.fillRect (rowR.withHeight (1).translated (0, rowR.getHeight() - 1));

        // Active indicator
        auto r = rowR.reduced (8, 0);
        if (isSelected)
        {
            g.setColour (kAmber);
            g.fillEllipse (Rectangle<float> ((float) r.getX() + 2, (float) rowR.getCentreY() - 3, 6, 6));
        }
        r.removeFromLeft (14);

        // Preset name
        const String name (p.name);
        g.setFont (nameFont());
        g.setColour (isSelected ? kAmberGlow : kCream2);
        const int nameW = jmin (200, GlyphArrangement::getStringWidthInt (nameFont(), name) + 4);
        g.drawText (name, r.removeFromLeft (nameW), Justification::centredLeft);
        r.removeFromLeft (8);

        // Formula + speed pills
        const Colour pillBg = formulaColour (p.tape_formula);
        const Colour pillFg = kCream2;
        auto pillArea = r.removeFromLeft (80);
        drawPill (g, pillArea, String (formulaLabel (p.tape_formula)), pillBg, pillFg);
        drawPill (g, pillArea, String (speedLabel (p.speed)) + " IPS",
                  Colour (0xff2a2a1a), kSilver);
        r.removeFromLeft (4);

        // Description
        g.setFont (descFont());
        g.setColour (kSoftGray);
        g.drawText (String (p.tip), r, Justification::centredLeft, true);
    }

    // ----------------------------------------------------------------
    void PresetBrowser::drawTabBar (Graphics& g) const
    {
        // Background
        g.setColour (Colour (0xff0e0c08));
        g.fillRect (0, 0, getWidth(), kTabH);

        // Separator
        g.setColour (Colour (0xff3a3020));
        g.fillRect (0, kTabH, getWidth(), 1);

        for (const auto& t : tabRects)
        {
            const bool sel = (t.catIdx == selectedCat);
            // Tab background
            if (sel)
            {
                g.setColour (kAmberDim.withAlpha (0.8f));
                g.fillRoundedRectangle (t.r.toFloat(), 3.0f);
            }
            // Tab label
            g.setColour (sel ? kAmberGlow : kSoftGray);
            g.setFont (tabFont());
            g.drawText (String (kCategories[t.catIdx]), t.r, Justification::centred);
        }
    }

    // ----------------------------------------------------------------
    void PresetBrowser::paint (Graphics& g)
    {
        // Overall background
        g.setColour (Colour (0xf0100e08));
        g.fillRect (getLocalBounds());

        // Outer border
        g.setColour (kAmberDim.withAlpha (0.6f));
        g.drawRect (getLocalBounds(), 1);

        drawTabBar (g);

        // List area — clip to it
        const auto la = listArea();
        {
            const Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (la);

            int y0 = la.getY() - (int) scrollOffset;
            for (int row = 0; row < filteredIndices.size(); ++row)
            {
                const int presetIdx = filteredIndices[row];
                const auto rowR = Rectangle<int> (la.getX(), y0 + row * kRowH,
                                                   la.getWidth() - kScrollW, kRowH);
                drawRow (g, rowR, presetIdx,
                         presetIdx == currentPreset,
                         row == hoverRow);
            }
        }

        // Scrollbar (if needed)
        if (maxScroll() > 0.0f)
        {
            const float totalH = (float) filteredIndices.size() * kRowH;
            const float viewH  = (float) la.getHeight();
            const float thumbH = jmax (20.0f, viewH / totalH * viewH);
            const float thumbY = (scrollOffset / maxScroll()) * (viewH - thumbH);

            auto laSb = la;
            const auto sbr = laSb.removeFromRight (kScrollW);
            g.setColour (Colour (0xff1e1a10));
            g.fillRect (sbr);
            g.setColour (kAmberDim);
            g.fillRoundedRectangle (
                (float) sbr.getX() + 2,
                (float) la.getY() + thumbY,
                (float) kScrollW - 4,
                thumbH,
                2.0f);
        }
    }

} // namespace bc2000dl::ui
