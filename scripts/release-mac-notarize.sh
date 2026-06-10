#!/usr/bin/env bash
# release-mac-notarize.sh — Developer ID-signera + notarisera + bygg .pkg-installerare.
#
# Detta är RELEASE-vägen (skild från scripts/package-dist.sh som ad-hoc-signerar
# för en kompis). Producerar en notariserad, stapled .pkg som Gatekeeper släpper
# igenom utan varningar på vilken Mac som helst.
#
# FÖRKRAV (certifikat + konton — finns INTE i repot, sätts som env/secrets):
#   DEV_ID_APP        "Developer ID Application: Soundboys ApS (TEAMID)"
#   DEV_ID_INSTALLER  "Developer ID Installer: Soundboys ApS (TEAMID)"
#   AC_NOTARY_PROFILE namn på keychain-profil skapad med:
#                       xcrun notarytool store-credentials AC_NOTARY_PROFILE \
#                         --apple-id you@soundboys.com --team-id TEAMID \
#                         --password <app-specific-password>
#
# Kör bygget FÖRST (universal):
#   cmake --build plugin/juce/build --target BC2000DL_All -j8
#
# Sedan:
#   DEV_ID_APP="Developer ID Application: …" \
#   DEV_ID_INSTALLER="Developer ID Installer: …" \
#   AC_NOTARY_PROFILE="AC_NOTARY_PROFILE" \
#   scripts/release-mac-notarize.sh
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$REPO/plugin/juce/build/BC2000DL_artefacts/Release"
VST3_SRC="$BUILD/VST3/Germanium 2000 Deluxe.vst3"
AU_SRC="$BUILD/AU/Germanium 2000 Deluxe.component"
PRODUCT="Germanium 2000 Deluxe"
VERSION="$(sed -n 's/^project(BC2000DL VERSION \([0-9.]*\)).*/\1/p' "$REPO/plugin/juce/CMakeLists.txt")"
OUT="$REPO/output/pkg"
PKG="$OUT/${PRODUCT// /_}_${VERSION}_macOS.pkg"

# ---- Preflight: certifikat måste finnas -------------------------------------
missing=0
for v in DEV_ID_APP DEV_ID_INSTALLER AC_NOTARY_PROFILE; do
    if [ -z "${!v:-}" ]; then echo "✗ saknar env: $v"; missing=1; fi
done
[ -d "$VST3_SRC" ] || { echo "✗ saknar VST3 ($VST3_SRC) — kör bygget först."; missing=1; }
[ -d "$AU_SRC" ]   || { echo "✗ saknar AU ($AU_SRC) — kör bygget först."; missing=1; }
if [ "$missing" = 1 ]; then
    echo
    echo "Detta script kräver Apple Developer ID-certifikat + en notarytool-profil."
    echo "Se docs/RELEASE_SIGNING.md för hur du sätter upp dem. Avbryter."
    exit 1
fi

mkdir -p "$OUT"
STAGE="$(mktemp -d)"

echo "==> (1/5) Kopierar + rensar xattr"
cp -R "$VST3_SRC" "$STAGE/"
cp -R "$AU_SRC"   "$STAGE/"
xattr -cr "$STAGE"/*

# ---- Hardened runtime-signering (krav för notarisering) ---------------------
echo "==> (2/5) Developer ID-signerar (hardened runtime + secure timestamp)"
for bundle in "$STAGE/$PRODUCT.vst3" "$STAGE/$PRODUCT.component"; do
    codesign --force --deep --options runtime --timestamp \
             --sign "$DEV_ID_APP" "$bundle"
    codesign --verify --deep --strict "$bundle"
done

# ---- Komponent-pkg per format → distributions-pkg ---------------------------
echo "==> (3/5) Bygger installer-pkg (signerad med Developer ID Installer)"
COMP_DIR="$(mktemp -d)"
pkgbuild --component "$STAGE/$PRODUCT.vst3" \
         --install-location "/Library/Audio/Plug-Ins/VST3" \
         "$COMP_DIR/vst3.pkg"
pkgbuild --component "$STAGE/$PRODUCT.component" \
         --install-location "/Library/Audio/Plug-Ins/Components" \
         "$COMP_DIR/au.pkg"

DIST="$(mktemp).xml"
cat > "$DIST" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>$PRODUCT $VERSION</title>
  <options customize="never" require-scripts="false"/>
  <pkg-ref id="com.soundboys.germanium2000.vst3"/>
  <pkg-ref id="com.soundboys.germanium2000.au"/>
  <choices-outline>
    <line choice="default"><line choice="vst3"/><line choice="au"/></line>
  </choices-outline>
  <choice id="default"/>
  <choice id="vst3" title="VST3"><pkg-ref id="com.soundboys.germanium2000.vst3"/></choice>
  <choice id="au"   title="Audio Unit"><pkg-ref id="com.soundboys.germanium2000.au"/></choice>
  <pkg-ref id="com.soundboys.germanium2000.vst3" version="$VERSION">vst3.pkg</pkg-ref>
  <pkg-ref id="com.soundboys.germanium2000.au"   version="$VERSION">au.pkg</pkg-ref>
</installer-gui-script>
XML

productbuild --distribution "$DIST" --package-path "$COMP_DIR" \
             --sign "$DEV_ID_INSTALLER" --timestamp "$PKG"

# ---- Notarisering + staple --------------------------------------------------
echo "==> (4/5) Skickar till Apple för notarisering (väntar)…"
xcrun notarytool submit "$PKG" --keychain-profile "$AC_NOTARY_PROFILE" --wait

echo "==> (5/5) Staplar notariseringsbevis"
xcrun stapler staple "$PKG"
xcrun stapler validate "$PKG" && echo "   staple OK"
spctl --assess --type install -vv "$PKG" || true

echo
echo "✓ Klar: $PKG"
