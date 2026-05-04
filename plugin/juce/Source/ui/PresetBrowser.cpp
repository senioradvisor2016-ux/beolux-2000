/*  PresetBrowser — maxed UAD-style preset browser. */

#include "PresetBrowser.h"
#include "BCColours.h"
#include <cstring>
#include <cctype>

namespace bc2000dl::ui
{
    using namespace juce;
    using namespace bc2000dl::ui::colours;

    // ------------------------------------------------------------------
    //  Palette (local to this file — BC2000 dark amber theme)
    // ------------------------------------------------------------------
    namespace pal
    {
        const Colour bg         { 0xff0f0d09 };  // panel background
        const Colour bgCat      { 0xff0b0906 };  // category column
        const Colour bgRow      { 0xff121008 };  // normal row
        const Colour bgHover    { 0xff1e1a12 };  // hovered row
        const Colour bgSelected { 0xff321e08 };  // selected row
        const Colour bgInfo     { 0xff090704 };  // info panel
        const Colour bgSearch   { 0xff181410 };  // search bar
        const Colour divider    { 0xff2e2618 };  // separating lines
        const Colour catSel     { 0xff2e1e06 };  // selected category bg
        const Colour catHover   { 0xff181410 };

        const Colour amber      { 0xffffb14a };
        const Colour amberGlow  { 0xffffd58a };
        const Colour amberDim   { 0xff5a4824 };
        const Colour cream      { 0xffefe6cf };
        const Colour creamDim   { 0xffd0c09a };
        const Colour gray       { 0xff8a7a5e };
        const Colour grayDim    { 0xff5a4e3a };

        // bar colours per parameter
        const Colour barDrive   { 0xffd4861e };  // warm amber
        const Colour barWow     { 0xff2a6a7a };  // teal
        const Colour barBias    { 0xffa06428 };  // orange-brown
        const Colour barBass    { 0xff3a6a2a };  // green
        const Colour barTreble  { 0xff5aaa4a };  // bright green
        const Colour barEcho    { 0xff4a3a8a };  // indigo

        // formula pill colours
        static Colour formulaBg (int f)
        {
            switch (f) { case 1: return Colour(0xff1e4a58); case 2: return Colour(0xff1e3a1e); default: return Colour(0xff4a2a08); }
        }
        static Colour formulaFg (int f)
        {
            switch (f) { case 1: return Colour(0xff6adaee); case 2: return Colour(0xff6ae88a); default: return Colour(0xffffb14a); }
        }
        static const char* formulaLabel (int f)
        { switch (f) { case 1: return "BASF"; case 2: return "SCOTCH"; default: return "AGFA"; } }
        static const char* speedLabel (int s)
        { switch (s) { case 0: return "4.75"; case 1: return "9.5"; default: return "19"; } }
    }

    // ------------------------------------------------------------------
    //  Fonts (also defined as free functions for non-member draw helpers)
    // ------------------------------------------------------------------
    static Font sPillFont()     { return Font (FontOptions {}.withHeight (10.0f).withStyle ("Bold")); }

    Font PresetBrowser::nameFont()     { return Font (FontOptions {}.withHeight (13.5f).withStyle ("Bold")); }
    Font PresetBrowser::catFont()      { return Font (FontOptions {}.withHeight (12.5f).withStyle ("Bold")); }
    Font PresetBrowser::pillFont()     { return sPillFont(); }
    Font PresetBrowser::descFont()     { return Font (FontOptions {}.withHeight (11.5f)); }
    Font PresetBrowser::infoNameFont() { return Font (FontOptions {}.withHeight (15.0f).withStyle ("Bold")); }
    Font PresetBrowser::paramFont()    { return Font (FontOptions {}.withHeight (10.0f)); }

    // ------------------------------------------------------------------
    //  Constructor
    // ------------------------------------------------------------------
    PresetBrowser::PresetBrowser()
    {
        setInterceptsMouseClicks (true, true);
        setWantsKeyboardFocus    (true);

        // Search box style
        searchBox.setMultiLine          (false);
        searchBox.setReturnKeyStartsNewLine (false);
        searchBox.setScrollbarsShown    (false);
        searchBox.setPopupMenuEnabled   (false);
        searchBox.setTextToShowWhenEmpty ("  🔍  Search presets...", Colour (0xff5a4e3a));
        searchBox.setFont               (Font (FontOptions {}.withHeight (13.5f)));
        searchBox.setColour (TextEditor::backgroundColourId,    pal::bgSearch);
        searchBox.setColour (TextEditor::outlineColourId,       Colours::transparentBlack);
        searchBox.setColour (TextEditor::focusedOutlineColourId,pal::amberDim);
        searchBox.setColour (TextEditor::textColourId,          pal::cream);
        searchBox.setColour (TextEditor::highlightColourId,     pal::amberDim.withAlpha(0.5f));
        searchBox.setColour (CaretComponent::caretColourId,      pal::amber);
        searchBox.addListener (this);
        addAndMakeVisible (searchBox);

        rebuildFilteredList();
    }

    // ------------------------------------------------------------------
    void PresetBrowser::openBrowser (int currentPresetIndex)
    {
        currentPreset = currentPresetIndex;
        infoPreset    = currentPresetIndex;
        selectedCat   = 0;
        scrollOffset  = 0.0f;
        hoverRow      = -1;
        searchText    = {};
        searchBox.clear();

        rebuildFilteredList();
        buildCatRows();

        // Auto-scroll to make current preset visible
        scrollToRow (filteredIndices.indexOf (currentPreset));

        setVisible  (true);
        toFront     (false);
        searchBox.grabKeyboardFocus();
        repaint();
    }

    // ------------------------------------------------------------------
    //  Layout zones
    // ------------------------------------------------------------------
    void PresetBrowser::resized()
    {
        auto b = getLocalBounds();
        zSearch = b.removeFromTop (kSearchH);
        zInfo   = b.removeFromBottom (kInfoH);
        zCat    = b.removeFromLeft (kCatW);
        zList   = b;                           // remaining = list + scrollbar

        // Search box fills zone minus close-button slot (26px right)
        searchBox.setBounds (zSearch.reduced (4, 4).withTrimmedRight (26));
        buildCatRows();
    }

    // ------------------------------------------------------------------
    //  Category helpers
    // ------------------------------------------------------------------
    void PresetBrowser::buildCatRows()
    {
        catRows.clear();
        const int h    = kSearchH == 0 ? 26 : 26;
        const int total = zCat.getHeight();
        const int rows  = kNumCategories;
        const int rowH  = jmin (h, total / rows);

        for (int i = 0; i < rows; ++i)
            catRows.add ({ i, { zCat.getX(), zCat.getY() + i * rowH, kCatW, rowH } });
    }

    int PresetBrowser::catAtPoint (Point<int> p) const
    {
        for (const auto& c : catRows)
            if (c.r.contains (p)) return c.catIdx;
        return -1;
    }

    int PresetBrowser::presetCountForCat (int catIdx) const
    {
        if (catIdx == 0) return kNumPresets;
        int count = 0;
        for (int i = 0; i < kNumPresets; ++i)
            if (std::strcmp (kPresets[i].category, kCategories[catIdx]) == 0)
                ++count;
        return count;
    }

    void PresetBrowser::selectCategory (int idx)
    {
        if (selectedCat == idx) return;
        selectedCat  = idx;
        scrollOffset = 0.0f;
        hoverRow     = -1;
        rebuildFilteredList();
        buildCatRows();
        repaint();
    }

    // ------------------------------------------------------------------
    //  List helpers
    // ------------------------------------------------------------------
    void PresetBrowser::rebuildFilteredList()
    {
        filteredIndices.clear();
        const bool hasSearch = searchText.isNotEmpty();
        const String sLow    = searchText.toLowerCase();

        for (int i = 0; i < kNumPresets; ++i)
        {
            const auto& p = kPresets[i];
            const bool inCat = (selectedCat == 0) ||
                               (std::strcmp (p.category, kCategories[selectedCat]) == 0);
            if (!inCat) continue;

            if (hasSearch)
            {
                const String name = String (p.name).toLowerCase();
                const String desc = String (p.tip).toLowerCase();
                const String cat  = String (p.category).toLowerCase();
                if (!name.contains (sLow) && !desc.contains (sLow) && !cat.contains (sLow))
                    continue;
            }
            filteredIndices.add (i);
        }
    }

    int PresetBrowser::rowAtY (int y) const
    {
        const int relY = y - zList.getY() + (int) scrollOffset;
        if (relY < 0) return -1;
        const int row = relY / kRowH;
        if (row < 0 || row >= filteredIndices.size()) return -1;
        return row;
    }

    float PresetBrowser::maxScroll() const
    {
        const float total = (float) filteredIndices.size() * kRowH;
        const float view  = (float) (zList.getHeight());
        return jmax (0.0f, total - view);
    }

    void PresetBrowser::clampScroll()
    {
        scrollOffset = jlimit (0.0f, maxScroll(), scrollOffset);
    }

    void PresetBrowser::scrollToRow (int row)
    {
        if (row < 0 || zList.isEmpty()) return;
        const float rowY = (float) row * kRowH;
        const float viewH = (float) zList.getHeight();
        if (rowY < scrollOffset)
            scrollOffset = rowY;
        else if (rowY + kRowH > scrollOffset + viewH)
            scrollOffset = rowY + kRowH - viewH;
        clampScroll();
    }

    // ------------------------------------------------------------------
    //  Mouse
    // ------------------------------------------------------------------
    void PresetBrowser::mouseDown (const MouseEvent& e)
    {
        const auto pt = e.getPosition();

        // ---- Close button (top-right corner) ----
        auto zSrch = zSearch;
        const auto closeR = zSrch.removeFromRight (26).reduced (6, 6);
        if (closeR.contains (pt)) { setVisible (false); return; }

        // ---- Category column ----
        if (zCat.contains (pt))
        {
            const int cat = catAtPoint (pt);
            if (cat >= 0) selectCategory (cat);
            return;
        }

        // ---- Preset list ----
        if (zList.contains (pt))
        {
            const int row = rowAtY (pt.y);
            if (row >= 0 && row < filteredIndices.size())
            {
                const int idx = filteredIndices[row];

                // Favourite toggle — last 30px of row
                auto rowR = Rectangle<int> (zList.getX(), zList.getY() + row * kRowH - (int) scrollOffset,
                                            zList.getWidth() - kScrollW, kRowH);
                const auto favR = rowR.removeFromRight (30);
                if (favR.contains (pt))
                {
                    if (favorites.count (idx)) favorites.erase (idx);
                    else                       favorites.insert (idx);
                    repaint (zList);
                    return;
                }

                // Apply preset
                currentPreset = idx;
                infoPreset    = idx;
                if (onPresetSelected) onPresetSelected (idx);
                setVisible (false);
                return;
            }
        }

        // ---- Anywhere else = close ----
        setVisible (false);
    }

    void PresetBrowser::mouseMove (const MouseEvent& e)
    {
        if (!zList.contains (e.getPosition()))
        {
            if (hoverRow != -1) { hoverRow = -1; infoPreset = currentPreset; repaint(); }
            return;
        }
        const int row = rowAtY (e.y);
        if (row != hoverRow)
        {
            hoverRow   = row;
            infoPreset = (row >= 0 && row < filteredIndices.size())
                         ? filteredIndices[row] : currentPreset;
            repaint();
        }
    }

    void PresetBrowser::mouseExit (const MouseEvent&)
    {
        if (hoverRow != -1) { hoverRow = -1; infoPreset = currentPreset; repaint(); }
    }

    void PresetBrowser::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
    {
        if (zList.contains (e.getPosition()) || zInfo.contains (e.getPosition()))
        {
            scrollOffset -= w.deltaY * 80.0f;
            clampScroll();
            repaint();
        }
    }

    // ------------------------------------------------------------------
    //  Keyboard
    // ------------------------------------------------------------------
    bool PresetBrowser::keyPressed (const KeyPress& k)
    {
        if (k == KeyPress::escapeKey) { setVisible (false); return true; }

        if (k == KeyPress::upKey || k == KeyPress::downKey)
        {
            const int cur  = filteredIndices.indexOf (currentPreset);
            const int next = jlimit (0, filteredIndices.size() - 1,
                                     cur + (k == KeyPress::downKey ? 1 : -1));
            if (next != cur)
            {
                currentPreset = filteredIndices[next];
                infoPreset    = currentPreset;
                if (onPresetSelected) onPresetSelected (currentPreset);
                scrollToRow (next);
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

    // ------------------------------------------------------------------
    //  TextEditor::Listener
    // ------------------------------------------------------------------
    void PresetBrowser::textEditorTextChanged (TextEditor& t)
    {
        searchText   = t.getText();
        scrollOffset = 0.0f;
        hoverRow     = -1;
        rebuildFilteredList();
        scrollToRow (filteredIndices.indexOf (currentPreset));
        repaint();
    }

    void PresetBrowser::textEditorReturnKeyPressed (TextEditor&) { setVisible (false); }
    void PresetBrowser::textEditorEscapeKeyPressed (TextEditor&) { setVisible (false); }

    // ====================================================================
    //  DRAWING
    // ====================================================================

    // ------------------------------------------------------------------
    //  Pill helper
    // ------------------------------------------------------------------
    static void drawPill (Graphics& g, Rectangle<int> r, const String& text,
                          Colour bg, Colour fg)
    {
        g.setColour (bg);
        g.fillRoundedRectangle (r.toFloat(), 3.0f);
        g.setColour (fg);
        g.setFont   (sPillFont());
        g.drawText  (text, r, Justification::centred);
    }

    // ------------------------------------------------------------------
    void PresetBrowser::paintParamBar (Graphics& g,
                                       Rectangle<int> area,
                                       const String& label,
                                       float norm,
                                       Colour barColour) const
    {
        auto r = area.reduced (2, 0);

        // Label
        g.setFont   (paramFont());
        g.setColour (pal::gray);
        g.drawText  (label, r.removeFromLeft (34), Justification::centredRight);
        r.removeFromLeft (4);

        // Track + fill
        const auto track = r;
        g.setColour (pal::divider.withAlpha (0.8f));
        g.fillRoundedRectangle (track.toFloat(), 2.0f);

        const int fillW = jmax (2, (int) (norm * track.getWidth()));
        g.setColour (barColour.withAlpha (0.85f));
        g.fillRoundedRectangle (track.withWidth (fillW).toFloat(), 2.0f);
    }

    // ------------------------------------------------------------------
    void PresetBrowser::paintRow (Graphics& g,
                                   Rectangle<int> rowR,
                                   int presetIdx,
                                   bool isSel,
                                   bool isHov) const
    {
        const auto& p = kPresets[presetIdx];
        const bool  isFav = favorites.count (presetIdx) > 0;

        // Background
        if (isSel)       g.setColour (pal::bgSelected);
        else if (isHov)  g.setColour (pal::bgHover);
        else             g.setColour (pal::bgRow);
        g.fillRect (rowR);

        // Left accent bar for selected
        if (isSel)
        {
            g.setColour (pal::amber);
            g.fillRect (rowR.withWidth (3));
        }

        // Bottom separator
        g.setColour (pal::divider);
        g.fillRect  (rowR.withTop (rowR.getBottom() - 1));

        // ---- Content ----
        auto r = rowR.reduced (10, 0).withTrimmedLeft (isSel ? 4 : 0);

        // Favourite button (right side, 28px)
        auto favR = r.removeFromRight (28).reduced (0, 11);
        if (isFav)
        {
            g.setColour (pal::amber);
            g.setFont   (Font (FontOptions {}.withHeight (14.0f)));
            g.drawText  ("♥", favR, Justification::centred);
        }
        else if (isHov || isSel)
        {
            g.setColour (pal::gray);
            g.setFont   (Font (FontOptions {}.withHeight (14.0f)));
            g.drawText  ("♡", favR, Justification::centred);
        }

        // Preset name
        const String nameStr (p.name);
        g.setFont   (nameFont());
        g.setColour (isSel ? pal::amberGlow : (isHov ? pal::cream : pal::creamDim));
        const int nw = jmin (220, GlyphArrangement::getStringWidthInt (nameFont(), nameStr) + 6);
        g.drawText  (nameStr, r.removeFromLeft (nw), Justification::centredLeft);
        r.removeFromLeft (8);

        // Formula pill
        const int fw = GlyphArrangement::getStringWidthInt (pillFont(), pal::formulaLabel (p.tape_formula)) + 12;
        drawPill (g, r.removeFromLeft (fw).reduced (0, 11),
                  pal::formulaLabel (p.tape_formula),
                  pal::formulaBg (p.tape_formula), pal::formulaFg (p.tape_formula));
        r.removeFromLeft (4);

        // Speed pill
        const String speedStr = String (pal::speedLabel (p.speed)) + " IPS";
        const int sw = GlyphArrangement::getStringWidthInt (pillFont(), speedStr) + 10;
        drawPill (g, r.removeFromLeft (sw).reduced (0, 11),
                  speedStr, Colour (0xff282010), pal::gray);
        r.removeFromLeft (8);

        // Description (remainder)
        g.setFont   (descFont());
        g.setColour (pal::gray.withAlpha (isHov || isSel ? 0.9f : 0.6f));
        g.drawText  (String (p.tip), r, Justification::centredLeft, true);
    }

    // ------------------------------------------------------------------
    void PresetBrowser::paintSearchBar (Graphics& g) const
    {
        // Background
        g.setColour (pal::bgSearch);
        g.fillRect  (zSearch);

        // Bottom separator
        g.setColour (pal::divider);
        g.fillRect  (zSearch.withTop (zSearch.getBottom() - 1));

        // Close button [✕]
        auto zSrchCopy = zSearch;
        const auto closeZone = zSrchCopy.removeFromRight (26);
        const auto closeR    = closeZone.reduced (7, 7);
        g.setColour (pal::gray);
        g.setFont   (Font (FontOptions {}.withHeight (11.0f)));
        g.drawText  ("✕", closeR, Justification::centred);
        // Separator line to the left of close zone
        g.setColour (pal::divider);
        g.fillRect  (zSrchCopy.removeFromRight (1));
    }

    // ------------------------------------------------------------------
    void PresetBrowser::paintCatColumn (Graphics& g) const
    {
        g.setColour (pal::bgCat);
        g.fillRect  (zCat);

        // Right border
        g.setColour (pal::divider);
        g.fillRect  (zCat.withLeft (zCat.getRight() - 1));

        for (const auto& c : catRows)
        {
            const bool isSel = (c.catIdx == selectedCat);
            const bool isHov = !isSel && c.r.contains (
                getLocalPoint (nullptr, Desktop::getInstance().getMainMouseSource().getScreenPosition()).roundToInt());

            if (isSel)
            {
                g.setColour (pal::catSel);
                g.fillRect  (c.r);
                // Left accent
                g.setColour (pal::amber);
                g.fillRect  (c.r.withWidth (3));
            }
            else if (isHov)
            {
                g.setColour (pal::catHover);
                g.fillRect  (c.r);
            }

            // Category name
            auto tr = c.r.reduced (12, 0);
            g.setFont   (catFont());
            g.setColour (isSel ? pal::amberGlow : pal::gray);
            const String catStr (kCategories[c.catIdx]);
            g.drawText  (catStr, tr, Justification::centredLeft);

            // Count badge
            const int count = presetCountForCat (c.catIdx);
            const String countStr = String (count);
            const int bw  = GlyphArrangement::getStringWidthInt (pillFont(), countStr) + 8;
            const auto br = tr.removeFromRight (bw).reduced (0, 9);
            g.setColour (isSel ? pal::amberDim : Colour (0xff201a12));
            g.fillRoundedRectangle (br.toFloat(), 3.0f);
            g.setColour (isSel ? pal::amber : pal::grayDim);
            g.setFont   (pillFont());
            g.drawText  (countStr, br, Justification::centred);

            // Row separator
            g.setColour (pal::divider.withAlpha (0.5f));
            g.fillRect  (c.r.withTop (c.r.getBottom() - 1));
        }

        // Unused space below categories
        if (!catRows.isEmpty())
        {
            const int lastY = catRows.getLast().r.getBottom();
            if (lastY < zCat.getBottom())
            {
                g.setColour (pal::bgCat);
                g.fillRect  (zCat.withTop (lastY));
            }
        }
    }

    // ------------------------------------------------------------------
    void PresetBrowser::paintListArea (Graphics& g) const
    {
        // Background
        g.setColour (pal::bgRow);
        g.fillRect  (zList);

        // ---- Preset rows (clipped) ----
        {
            const Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (zList.withTrimmedRight (kScrollW));

            const int y0   = zList.getY() - (int) scrollOffset;
            const int nRow = filteredIndices.size();
            for (int row = 0; row < nRow; ++row)
            {
                const int presetIdx = filteredIndices[row];
                const auto rowR = Rectangle<int> (
                    zList.getX(),
                    y0 + row * kRowH,
                    zList.getWidth() - kScrollW,
                    kRowH);

                // Skip rows fully outside clip
                if (rowR.getBottom() < zList.getY()) continue;
                if (rowR.getY() > zList.getBottom()) break;

                paintRow (g, rowR, presetIdx,
                          presetIdx == currentPreset,
                          row == hoverRow);
            }

            // Empty state
            if (nRow == 0)
            {
                g.setFont   (descFont());
                g.setColour (pal::grayDim);
                g.drawText  (searchText.isNotEmpty()
                                 ? "No presets match \"" + searchText + "\""
                                 : "No presets in this category",
                             zList.reduced (20), Justification::centred);
            }
        }

        // ---- Scrollbar ----
        if (maxScroll() > 0.0f)
        {
            auto zListCopy = zList;
            const auto sbr = zListCopy.removeFromRight (kScrollW);
            g.setColour (Colour (0xff181410));
            g.fillRect  (sbr);

            const float totalH = (float) filteredIndices.size() * kRowH;
            const float viewH  = (float) zList.getHeight();
            const float thumbH = jmax (16.0f, viewH / totalH * viewH);
            const float thumbY = (maxScroll() > 0.0f)
                                 ? (scrollOffset / maxScroll()) * (viewH - thumbH)
                                 : 0.0f;
            g.setColour (pal::amberDim);
            g.fillRoundedRectangle ((float) sbr.getX() + 2,
                                    (float) zList.getY() + thumbY,
                                    (float) kScrollW - 4,
                                    thumbH, 2.0f);
        }
    }

    // ------------------------------------------------------------------
    void PresetBrowser::paintInfoPanel (Graphics& g) const
    {
        const auto& p = kPresets[jlimit (0, kNumPresets - 1, infoPreset)];

        // Background
        g.setColour (pal::bgInfo);
        g.fillRect  (zInfo);

        // Top separator
        g.setColour (pal::divider);
        g.fillRect  (zInfo.withHeight (1));

        const auto inner = zInfo.reduced (10, 6);

        // ---- Left half: name + desc ----
        const int halfW = inner.getWidth() / 2;
        auto left = inner.withWidth (halfW - 8);

        // Name (large, amber when selected)
        const String nameStr (p.name);
        g.setFont   (infoNameFont());
        g.setColour (infoPreset == currentPreset ? pal::amberGlow : pal::cream);
        g.drawText  (nameStr, left.removeFromTop (22), Justification::centredLeft);

        // Pills on same line as name
        // (drawn inline after name)
        // Description
        g.setFont   (descFont());
        g.setColour (pal::gray);
        g.drawText  (String (p.tip), left, Justification::topLeft, true);

        // ---- Right half: 6 parameter bars ----
        auto right = inner.withX (inner.getX() + halfW).withWidth (halfW + 8);

        // Normalise param values
        const float normDrive  = jlimit (0.0f, 1.0f, (p.saturation_drive  - 0.5f) / 1.5f);
        const float normWow    = jlimit (0.0f, 1.0f,  p.wow_flutter / 2.0f);
        const float normBias   = jlimit (0.0f, 1.0f, (p.bias_amount - 0.5f) / 1.0f);
        const float normBass   = jlimit (0.0f, 1.0f, (p.bass_db + 12.0f)   / 24.0f);
        const float normTreble = jlimit (0.0f, 1.0f, (p.treble_db + 12.0f) / 24.0f);
        const float normEcho   = jlimit (0.0f, 1.0f,  p.echo_amount);

        const int barH = (right.getHeight() - 2) / 2;
        const int barW = right.getWidth() / 3;

        auto row1 = right.withHeight (barH);
        auto row2 = right.withTop (right.getY() + barH + 2).withHeight (barH);

        paintParamBar (g, row1.removeFromLeft (barW), "DRV", normDrive,  pal::barDrive);
        paintParamBar (g, row1.removeFromLeft (barW), "WOW", normWow,    pal::barWow);
        paintParamBar (g, row1,                       "BIA", normBias,   pal::barBias);
        paintParamBar (g, row2.removeFromLeft (barW), "BAS", normBass,   pal::barBass);
        paintParamBar (g, row2.removeFromLeft (barW), "TRB", normTreble, pal::barTreble);
        paintParamBar (g, row2,                       "ECH", normEcho,   pal::barEcho);
    }

    // ------------------------------------------------------------------
    void PresetBrowser::paint (Graphics& g)
    {
        // Base background + outer border
        g.setColour (pal::bg);
        g.fillRect  (getLocalBounds());
        g.setColour (pal::amberDim.withAlpha (0.5f));
        g.drawRect  (getLocalBounds(), 1);

        paintSearchBar (g);
        paintCatColumn (g);
        paintListArea  (g);
        paintInfoPanel (g);
    }

} // namespace bc2000dl::ui
