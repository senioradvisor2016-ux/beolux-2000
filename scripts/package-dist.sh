#!/usr/bin/env bash
# package-dist.sh — paketera Beolux 2000 för distribution till en kompis.
#
# Tar de BYGGDA universal-artefakterna (AU .component + VST3), ad-hoc-signerar
# dem, lägger till en Install.command och zippar till skrivbordet.
#
#   scripts/package-dist.sh
#
# Förkrav: kör bygget först (universal, macOS 14):
#   cmake --build plugin/juce/build --target BC2000DL_All -j8
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$REPO/plugin/juce/build/BC2000DL_artefacts/Release"
VST3_SRC="$BUILD/VST3/Beolux 2000.vst3"
AU_SRC="$BUILD/AU/Beolux 2000.component"
ZIP="$HOME/Desktop/Beolux 2000 (AU + VST3) — macOS14 universal.zip"

[ -d "$VST3_SRC" ] || { echo "Saknar VST3: $VST3_SRC — kör bygget först."; exit 1; }
[ -d "$AU_SRC" ]   || { echo "Saknar AU: $AU_SRC — kör bygget först."; exit 1; }

STAGE="$(mktemp -d)/Beolux 2000"
mkdir -p "$STAGE"
echo "==> Kopierar artefakter till staging"
cp -R "$VST3_SRC" "$STAGE/"
cp -R "$AU_SRC"   "$STAGE/"

echo "==> Rensar extended attributes"
xattr -cr "$STAGE/Beolux 2000.vst3" "$STAGE/Beolux 2000.component" 2>/dev/null || true

echo "==> Ad-hoc-signerar (universal)"
codesign --force --deep --sign - "$STAGE/Beolux 2000.vst3"
codesign --force --deep --sign - "$STAGE/Beolux 2000.component"
codesign --verify --deep --strict "$STAGE/Beolux 2000.vst3"     && echo "   VST3 signatur OK"
codesign --verify --deep --strict "$STAGE/Beolux 2000.component" && echo "   AU signatur OK"

echo "==> Skriver Install.command"
cat > "$STAGE/Install.command" <<'INSTALL'
#!/bin/bash
# Beolux 2000 — installerare (AU + VST3, universal, macOS 14+)
set -e
DIR="$(cd "$(dirname "$0")" && pwd)"
VST3="$HOME/Library/Audio/Plug-Ins/VST3"
COMP="$HOME/Library/Audio/Plug-Ins/Components"
echo "Installerar Beolux 2000 (AU + VST3)..."
mkdir -p "$VST3" "$COMP"
rm -rf "$VST3/Beolux 2000.vst3" "$COMP/Beolux 2000.component"
cp -R "$DIR/Beolux 2000.vst3"      "$VST3/"
cp -R "$DIR/Beolux 2000.component" "$COMP/"
# Rensa Gatekeeper-quarantine så pluggen laddas (ad-hoc-signerad)
xattr -dr com.apple.quarantine "$VST3/Beolux 2000.vst3"      2>/dev/null || true
xattr -dr com.apple.quarantine "$COMP/Beolux 2000.component" 2>/dev/null || true
echo ""
echo "KLART. Starta om din DAW och sök efter 'Beolux 2000'."
echo "  VST3 → $VST3"
echo "  AU   → $COMP"
read -n1 -s -r -p "Tryck valfri tangent för att stänga."
echo ""
INSTALL
chmod +x "$STAGE/Install.command"

echo "==> Zippar till skrivbordet"
rm -f "$ZIP"
( cd "$(dirname "$STAGE")" && ditto -c -k --sequesterRsrc --keepParent "Beolux 2000" "$ZIP" )

echo "==> Klart:"
ls -lh "$ZIP" | awk '{print "   "$5"  "$9}'
unzip -l "$ZIP" | awk 'NR>3 && NF>=4 {print "   "$4}' | grep -vE '/$' | head -20
rm -rf "$(dirname "$STAGE")"
