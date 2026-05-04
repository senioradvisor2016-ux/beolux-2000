#!/usr/bin/env bash
# dev-reaper.sh — build the Beolux 2000 plugin and relaunch REAPER.
#
# Usage:
#   scripts/dev-reaper.sh                # build + relaunch REAPER
#   scripts/dev-reaper.sh --no-launch    # build only, leave REAPER alone
#   scripts/dev-reaper.sh --clean        # rm -rf build dir, then build
#   scripts/dev-reaper.sh --force-quit   # kill REAPER if graceful quit hangs
#   CONFIG=Debug scripts/dev-reaper.sh   # build Debug instead of Release

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SOURCE_DIR="$REPO_ROOT/plugin/juce"
BUILD_DIR="$SOURCE_DIR/build"
REAPER_APP="/Applications/REAPER.app"
TARGET="BC2000DL"
CONFIG="${CONFIG:-Release}"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

LAUNCH=true
CLEAN=false
FORCE_QUIT=false
for arg in "$@"; do
  case "$arg" in
    --no-launch)  LAUNCH=false ;;
    --clean)      CLEAN=true ;;
    --force-quit) FORCE_QUIT=true ;;
    -h|--help)
      sed -n '2,9p' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *)
      echo "Unknown arg: $arg" >&2
      exit 1
      ;;
  esac
done

if [ ! -d "$SOURCE_DIR" ]; then
  echo "==> Source dir not found: $SOURCE_DIR" >&2
  exit 1
fi

if $CLEAN; then
  echo "==> Cleaning $BUILD_DIR"
  rm -rf "$BUILD_DIR"
fi

need_configure=false
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
  need_configure=true
else
  cached_home="$(grep -E '^CMAKE_HOME_DIRECTORY:INTERNAL=' "$BUILD_DIR/CMakeCache.txt" | cut -d= -f2-)"
  if [ "$cached_home" != "$SOURCE_DIR" ]; then
    echo "==> Stale CMakeCache (configured for $cached_home); reconfiguring"
    rm -rf "$BUILD_DIR"
    need_configure=true
  fi
fi
if $need_configure; then
  echo "==> Configuring CMake ($CONFIG)"
  cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CONFIG"
fi

echo "==> Building $TARGET ($CONFIG, -j$JOBS)"
cmake --build "$BUILD_DIR" --target "$TARGET" --config "$CONFIG" -j "$JOBS"

if ! $LAUNCH; then
  echo "==> --no-launch given; done."
  exit 0
fi

if pgrep -x REAPER >/dev/null; then
  echo "==> Quitting REAPER (so it reloads the new dylib)"
  osascript -e 'tell application "REAPER" to quit' >/dev/null 2>&1 || true
  for _ in $(seq 1 20); do
    pgrep -x REAPER >/dev/null || break
    sleep 0.25
  done
  if pgrep -x REAPER >/dev/null; then
    if $FORCE_QUIT; then
      echo "==> Force-killing REAPER"
      pkill -x REAPER || true
      sleep 0.5
    else
      echo "==> REAPER didn't quit (likely an unsaved-changes dialog)." >&2
      echo "    Save your work and re-run, or pass --force-quit." >&2
      exit 1
    fi
  fi
fi

echo "==> Launching REAPER"
open "$REAPER_APP"
