# BC2000DL — Danish Tape 2000

VST3 / AU-plugin som emulerar Bang & Olufsen BeoCord 2000 De Luxe (1967–69), en 3-hastighets stereorullbandspelare med separata record/playback-huvuden, germaniumtransistorer, 100 kHz bias och DIN-1962 EQ.

**Status:** Fas 0–1 (komponentanalys + DSP-prototyp). Inte spelbar än.

## Disclaimer

This is an independent emulation project. **Not affiliated with, endorsed by, or sponsored by Bang & Olufsen A/S.** "BeoCord", "BeoGram", "Beogram", and "B&O" are trademarks of their respective owner. The product name in this project is **BC2000DL** / **Danish Tape 2000**.

## Mappstruktur

```
plugin/
├── README.md             ← detta dokument
├── specs.md              ← numeriska mätmål (Fas 0)
├── dsp_prototype/        ← Python/numpy-prototyp (Fas 1)
│   ├── ge_stages.py      ← germanium-transistor-stages
│   ├── test_ge_stages.py ← validering mot databladet
│   └── requirements.txt
└── (kommande)
    ├── juce/             ← JUCE-skelett (Fas 2)
    ├── ui/               ← skin assets (Fas 3)
    └── presets/          ← 1968 Manual Presets (Fas 3)
```

## Källmaterial

Schemat, servicemanual och periodtidskrifter ligger i `../Schema/`. Användarmanualen i danska finns på Dropbox-länken som dokumenterats i planfilen `~/.claude/plans/analysera-hela-appens-filer-purring-toucan.md`.

## Bygginstruktioner

(Kommer i Fas 2 när JUCE-skelettet är på plats.)
