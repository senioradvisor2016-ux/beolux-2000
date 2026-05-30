#!/usr/bin/env bash
# coverage.sh — mät DSP-kodtäckning (llvm source-based coverage).
#
# Bygger de fem DSP-enhetstestmålen med -fprofile-instr-generate
# -fcoverage-mapping, kör dem, slår ihop profilerna och rapporterar
# rad/region/funktions-täckning för Source/dsp/ (+ valfri HTML).
#
# Usage:
#   scripts/coverage.sh             # rapport i terminalen
#   scripts/coverage.sh --html      # + HTML-rapport i build-cov/cov-html/index.html
#
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$REPO_ROOT/plugin/juce"
BUILD="$SRC/build-cov"
PROF="$BUILD/profraw"
HTML=false
[ "${1:-}" = "--html" ] && HTML=true

PROFDATA="$(xcrun --find llvm-profdata)"
COV="$(xcrun --find llvm-cov)"

echo "==> Konfigurerar coverage-bygge"
cmake -S "$SRC" -B "$BUILD" -DBC2000DL_COVERAGE=ON \
      -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_BUILD_TYPE=Debug >/dev/null

echo "==> Bygger DSP-testmålen"
cmake --build "$BUILD" \
      --target BC2000DL_Catch2 BC2000DL_GTest BC2000DL_Tests \
               BC2000DL_BugFixTests BC2000DL_AudibilityTest \
      -j"$(sysctl -n hw.ncpu)" >/dev/null

# målnamn → binärsökväg (AudibilityTest har mellanslag i PRODUCT_NAME)
declare -a BINS=(
  "$BUILD/BC2000DL_Catch2_artefacts/Debug/BC2000DL_Catch2"
  "$BUILD/BC2000DL_GTest_artefacts/Debug/BC2000DL_GTest"
  "$BUILD/BC2000DL_Tests_artefacts/Debug/BC2000DL_Tests"
  "$BUILD/BC2000DL_BugFixTests_artefacts/Debug/BC2000DL_BugFixTests"
  "$BUILD/BC2000DL_AudibilityTest_artefacts/Debug/BC2000DL AudibilityTest"
)

rm -rf "$PROF"; mkdir -p "$PROF"
echo "==> Kör testmålen (instrumenterade)"
i=0
for b in "${BINS[@]}"; do
  if [ -x "$b" ]; then
    LLVM_PROFILE_FILE="$PROF/t$i.profraw" "$b" >/dev/null 2>&1 || true
    i=$((i+1))
  else
    echo "   (saknas: $b)" >&2
  fi
done

echo "==> Slår ihop profiler"
"$PROFDATA" merge -sparse "$PROF"/*.profraw -o "$PROF/merged.profdata"

# llvm-cov behöver ett bin + extra via -object för korrekt aggregering
OBJ_ARGS=()
first=true
for b in "${BINS[@]}"; do
  [ -x "$b" ] || continue
  if $first; then MAIN="$b"; first=false; else OBJ_ARGS+=( -object "$b" ); fi
done

echo
echo "================  DSP-TÄCKNING (Source/dsp)  ================"
"$COV" report "$MAIN" "${OBJ_ARGS[@]}" \
    -instr-profile="$PROF/merged.profdata" \
    "$SRC/Source/dsp" 2>/dev/null

if $HTML; then
  echo
  echo "==> Genererar HTML"
  "$COV" show "$MAIN" "${OBJ_ARGS[@]}" \
      -instr-profile="$PROF/merged.profdata" \
      -format=html -output-dir="$BUILD/cov-html" \
      "$SRC/Source/dsp" >/dev/null 2>&1
  echo "   $BUILD/cov-html/index.html"
fi
