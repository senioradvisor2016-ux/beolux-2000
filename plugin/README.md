# BC2000DL — Danish Tape 2000

VST3 / AU / Standalone-plugin som emulerar Bang & Olufsen BeoCord 2000 De Luxe (1967–69) — en 3-hastighets stereorullbandspelare med separata record/playback-huvuden, germaniumtransistorer, 100 kHz bias och DIN-1962 EQ.

**Status:** v28 (FullSpec) produktionsklar — VST3 + AU + Standalone, universal macOS 14+ (arm64 + x86_64). Validering: 22/22 mätningar pass mot Studio-Sound 1968 + service manual.

## Disclaimer

This is an independent emulation project. **Not affiliated with, endorsed by, or sponsored by Bang & Olufsen A/S.** "BeoCord", "BeoGram", "Beogram", and "B&O" are trademarks of their respective owner. The product name in this project is **BC2000DL** / **Danish Tape 2000**.

## Snabbstart

**Kör Standalone direkt:**
```
open output/v28/BC2000DL.app
```

**Installera som VST3/AU i en DAW:**
```
cp -R output/v28/BC2000DL.vst3      ~/Library/Audio/Plug-Ins/VST3/
cp -R output/v28/BC2000DL.component ~/Library/Audio/Plug-Ins/Components/
```

## Mappstruktur

```
plugin/
├── README.md                 ← detta dokument
├── specs.md                  ← numeriska mätmål (DIN 1962, Studio-Sound 1968)
├── VALIDATION_REPORT.md      ← 22/22 mätningar pass
├── UX_BRIEF.md               ← design-intent (438 rader)
├── FIGMA_UX_PROMPT.md        ← design-system-spec (472 rader)
├── juce/                     ← C++/JUCE-pluginet (källkod + CMakeLists)
│   ├── CMakeLists.txt
│   └── Source/
│       ├── PluginProcessor.{cpp,h}
│       ├── PluginEditor.{cpp,h}     ← native JUCE-editor (HybridHeroPanel)
│       ├── dsp/                     ← 11 DSP-moduler (~2 250 LOC)
│       └── ui/                      ← 9 UI-klasser + 22 PNG-assets
├── dsp_prototype/            ← Python/numpy-referensimplementation
├── diagrams/                 ← validerings-plots (DIN-1962 EQ, hysteresis)
├── mockups/                  ← design-mockups
├── output/
│   ├── v28/                              ← release: VST3 + AU + Standalone
│   ├── BC2000DL-v28-FullSpec-macOS14.zip ← distributionsbar zip
│   ├── BC2000DL_Instruktionsbog.pptx     ← användarmanual
│   └── BC2000DL_Retro_Presentation.pptx
├── build_manual_deck.py      ← genererar Instruktionsbog.pptx
└── build_retro_deck.py       ← genererar Retro_Presentation.pptx
```

## Källmaterial

Schemat, servicemanual och periodtidskrifter ligger i [../Schema/](../Schema/) (~180 MB): B&O Beocord 2000 service manual, Studio-Sound 1968-07/08/09, 2N2613-datablad, annoterade PCB-foton.

## Bygga från källa

Kräver CMake 3.22+, Xcode-CLT, JUCE 8.0.6 (hämtas automatiskt via FetchContent).

```
cd juce
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Artefakter hamnar i `juce/build/BC2000DL_artefacts/Release/{VST3,AU,Standalone}/`.

## DSP-pipeline

Per kanal: Mic/Phono/Radio-preamp → germanium-stage (2N2613, Ebers-Moll) → DIN-1962 pre-emphasis → tape-saturation (Jiles-Atherton hysteresis + 100 kHz bias) → wow/flutter → playback EQ + head-bump → echo (per-speed delay) → multiplay-bounce → tone/balance → soft-clip power-amp.
