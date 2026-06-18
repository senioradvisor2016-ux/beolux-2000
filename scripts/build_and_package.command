#!/usr/bin/env bash
# build_and_package.command — ETT dubbelklick: bygg + ad-hoc-signera + zippa.
#
# Dubbelklicka i Finder (eller kör i Terminal). Detta är BARA en bekvämlighets-
# wrapper runt det befintliga flödet:
#   1) konfigurera + bygg universal-pluginet (AU + VST3 + Standalone, macOS 14+)
#   2) scripts/package-dist.sh  → ad-hoc-signerar + skriver Install.command
#      + zippar till ~/Desktop
#
# Kräver macOS med Xcode Command Line Tools + CMake. JUCE hämtas automatiskt
# via FetchContent vid första bygget (kräver internet då).
#
# OBS: Detta är ad-hoc-signering (för egen maskin / en kompis). För en
# notariserad release-.pkg till vem som helst, använd scripts/release-mac-notarize.sh.
set -euo pipefail

# Resolva repo-roten utifrån scriptets egen plats (double-click → cwd = $HOME)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$SCRIPT_DIR/.." && pwd)"
SRC="$REPO/plugin/juce"
BUILD="$SRC/build"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 8)"

if [[ "$(uname)" != "Darwin" ]]; then
    echo "✗ Detta script kräver macOS (codesign/ditto/universal-bygge är macOS-only)."
    exit 1
fi

echo "==> (1/3) Konfigurerar (Release, universal arm64+x86_64)"
cmake -S "$SRC" -B "$BUILD" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"

echo "==> (2/3) Bygger plugin + alla mål (-j$JOBS)"
cmake --build "$BUILD" --config Release --target BC2000DL_All --parallel "$JOBS"

echo "==> (3/3) Paketerar (ad-hoc-signering + Install.command + zip → skrivbordet)"
"$REPO/scripts/package-dist.sh"

echo
echo "✓ Klart. Zippen ligger på ~/Desktop."
read -n1 -s -r -p "Tryck valfri tangent för att stänga." 2>/dev/null || true
echo
