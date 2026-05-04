#!/usr/bin/env bash
# validate.sh — run pluginval against the installed Beolux 2000 VST3 + AU.
#
# Usage:
#   scripts/validate.sh                  # default: strictness 5, both formats
#   scripts/validate.sh --strict 10      # strictness 10 (includes fuzzing — slow)
#   scripts/validate.sh --vst3-only      # skip AU
#   scripts/validate.sh --au-only        # skip VST3
#   scripts/validate.sh --gui            # include GUI tests (needs unlocked screen)
#
# Exit code: 0 if all checked formats pass, 1 otherwise.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PLUGINVAL="/Applications/pluginval.app/Contents/MacOS/pluginval"
VST3_PATH="$HOME/Library/Audio/Plug-Ins/VST3/Beolux 2000.vst3"
AU_PATH="$HOME/Library/Audio/Plug-Ins/Components/Beolux 2000.component"
LOG_DIR="$REPO_ROOT/output/pluginval"

STRICT=5
DO_VST3=true
DO_AU=true
SKIP_GUI=true
for arg in "$@"; do
  case "$arg" in
    --strict)     ;;
    --strict=*)   STRICT="${arg#*=}" ;;
    --vst3-only)  DO_AU=false ;;
    --au-only)    DO_VST3=false ;;
    --gui)        SKIP_GUI=false ;;
    -h|--help)
      sed -n '2,11p' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *)
      if [[ "${prev_arg:-}" == "--strict" ]]; then
        STRICT="$arg"
      else
        echo "Unknown arg: $arg" >&2; exit 1
      fi
      ;;
  esac
  prev_arg="$arg"
done

if [ ! -x "$PLUGINVAL" ]; then
  echo "pluginval not found at $PLUGINVAL" >&2
  echo "Install: brew install --cask pluginval" >&2
  exit 1
fi

mkdir -p "$LOG_DIR"

common_args=( --strictness-level "$STRICT" --timeout-ms 60000 --output-dir "$LOG_DIR" )
$SKIP_GUI && common_args+=( --skip-gui-tests )

run_one() {
  local label="$1" path="$2"
  echo "==> $label  (strictness=$STRICT)"
  if [ ! -e "$path" ]; then
    echo "    MISSING: $path  — build the plugin first (scripts/dev-reaper.sh --no-launch)" >&2
    return 1
  fi
  if "$PLUGINVAL" "${common_args[@]}" --validate "$path" >/tmp/pluginval-$$.log 2>&1; then
    grep -E '^(WARN|!!!|INFO: Skipping)' /tmp/pluginval-$$.log || true
    echo "    PASS"
    rm -f /tmp/pluginval-$$.log
    return 0
  else
    echo "    FAIL — full log:"
    tail -40 /tmp/pluginval-$$.log >&2
    rm -f /tmp/pluginval-$$.log
    return 1
  fi
}

failed=0
$DO_VST3 && { run_one "VST3" "$VST3_PATH" || failed=1; }
$DO_AU   && { run_one "AU"   "$AU_PATH"   || failed=1; }

echo
if [ $failed -eq 0 ]; then
  echo "All formats validated. Logs: $LOG_DIR"
else
  echo "One or more formats FAILED. Logs: $LOG_DIR" >&2
  exit 1
fi
