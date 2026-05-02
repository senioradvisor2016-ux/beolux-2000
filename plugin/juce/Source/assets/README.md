# BC2000DL — Asset Drop-In

Här kan du lägga in högkvalitets-PNG-assets som ersätter den procedural
renderingen för fotorealistisk look.

## Filnamn (förväntade av PhotorealReel-koden)

```
reel_left.png          — vänster reel (full med brun tape) — kvadratisk PNG
reel_right.png         — höger reel (tom EMITAPE-style)    — kvadratisk PNG
panel_surface.png      — control-panel bakgrund (charcoal grey)
deck_frame.png         — aluminum-deck-ramen runt reels
vu_meter.png           — VU-meter sprite (utan visare)
button_off.png         — push-knapp i opress-state
button_on.png          — push-knapp i pressed-state
knob_strip_60.png      — rotary-knob filmstrip (60 frames vertikalt)
```

## Hur du genererar assets från Sketchfab-modell

### 1. Ladda Sketchfab Reel-to-Reel free model
https://sketchfab.com/3d-models/reel-to-reel-tape-recorder-160ba370260b4f2b87effeaa0765f705
→ Klicka "Download 3D Model" → välj **glTF (.glb)**

### 2. Öppna i Blender (gratis)
- File → Import → glTF 2.0
- Selektera bara reel-mesh, ta bort allt annat
- Top-down kamera-view (Numpad 7)
- Sätt ortografisk projection
- Render-storlek: 1024 × 1024 px

### 3. Exportera reel_left.png + reel_right.png
- För reel_left: applicera brun tape-material i Blender, render
- För reel_right: ren acryl-material, render
- Båda: transparent BG (RGBA, alpha-channel)

### 4. Lägg PNG:erna i denna mapp

### 5. Aktivera assets i CMakeLists
```cmake
juce_add_binary_data(BC2000DL_PhotorealAssets SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/reel_left.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/assets/reel_right.png
    # … fler filer
)
target_link_libraries(BC2000DL PRIVATE BC2000DL_PhotorealAssets)
```

### 6. Aktivera lookup i PhotorealReel::tryLoadAsset()
Bygg om — JUCE laddar automatiskt PNG istället för procedural.

## Snabb-test med fritt CC0-asset

För quick-test utan Blender:
1. Ladda från [Wikimedia Commons Reel-to-Reel SVG](https://commons.wikimedia.org/wiki/File:Reel_to_reel_tape_recorder.svg)
2. Öppna i Inkscape, exportera som PNG @ 1024×1024
3. Klipp ut reels, spara som reel_left.png + reel_right.png
4. Drop här

## Filmstrip för roterande knobs (UAD-stil)

För bästa knob-rendering (matchar UAD/Soundtoys):
1. Rendera knob-rotation 0° → 270° i 60 frames
2. Stacka vertikalt till en enda PNG (60 × frame_height tall)
3. Spara som `knob_strip_60.png`
4. JUCE laddar och visar rätt frame baserat på slider-värde

Pre-byggda knob-filmstrips finns gratis på [HISE Community](https://forum.hise.audio/topic/6427/).
