#!/usr/bin/env bash
# reaper-render-calib.sh — kör specrig-kalibreringen genom Germanium 2000 Deluxe
# i en RIKTIG REAPER (headless via __startup.lua) och skriver renders/out_reaper.wav.
#
# Drar specrig/signals/calib_pass.wav genom ett spår med pluggen via en audio
# accessor (post-FX), läser hela längden och skriver en 16-bit PCM WAV som
# analyze.py kan mäta. Tape Speed lämnas på default (19 cm/s).
#
# Städar ALLTID bort __startup.lua (trap). Exit 0 om WAV skrevs.
set -uo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RES_DIR="$HOME/Library/Application Support/REAPER"
STARTUP="$RES_DIR/Scripts/__startup.lua"
STARTUP_BAK="$RES_DIR/Scripts/__startup.lua.bc2000bak"
CALIB="$REPO_ROOT/plugin/specrig/signals/calib_pass.wav"
OUT_WAV="$REPO_ROOT/plugin/specrig/renders/out_reaper.wav"
DONE="/tmp/bc2000_reaper_render_done.txt"
REAPER_BIN="/Applications/REAPER.app/Contents/MacOS/REAPER"
PLUGIN_NAME="${BC2000_PLUGIN_NAME:-Germanium 2000 Deluxe}"

cleanup() {
  if [ -f "$STARTUP_BAK" ]; then mv -f "$STARTUP_BAK" "$STARTUP"; else rm -f "$STARTUP"; fi
  rm -f /tmp/bc2000_render.RPP
  if pgrep -x REAPER >/dev/null 2>&1; then
    osascript -e 'tell application "REAPER" to quit' >/dev/null 2>&1 || true
    for _ in $(seq 1 40); do pgrep -x REAPER >/dev/null 2>&1 || break; sleep 0.25; done
    pkill -x REAPER >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

[ -x "$REAPER_BIN" ] || { echo "REAPER saknas: $REAPER_BIN"; exit 1; }
[ -f "$CALIB" ]      || { echo "calib saknas: $CALIB"; exit 1; }

echo "==> Installerar __startup.lua (render-variant)"
[ -f "$STARTUP" ] && mv -f "$STARTUP" "$STARTUP_BAK"
mkdir -p "$RES_DIR/Scripts"
cat > "$STARTUP" <<LUA
local CALIB = "$CALIB"
local OUT   = "$OUT_WAV"
local DONE  = "$DONE"
local PLUG  = "$PLUGIN_NAME"

local function writedone(s) local f=io.open(DONE,"w"); if f then f:write(s); f:close() end end

local function write_wav16(path, buf, total, nch, sr)
  local f = io.open(path, "wb"); if not f then return false end
  local datalen = total * 2                       -- total = nch*nframes, 2 byte/sample
  f:write("RIFF"); f:write(string.pack("<I4", 36 + datalen)); f:write("WAVE")
  f:write("fmt "); f:write(string.pack("<I4", 16))
  f:write(string.pack("<I2", 1))                  -- PCM
  f:write(string.pack("<I2", nch))
  f:write(string.pack("<I4", sr))
  f:write(string.pack("<I4", sr * nch * 2))       -- byterate
  f:write(string.pack("<I2", nch * 2))            -- blockalign
  f:write(string.pack("<I2", 16))                 -- bits
  f:write("data"); f:write(string.pack("<I4", datalen))
  local CH = 8192; local out = {}
  for i = 1, total do
    local v = buf[i]; if v ~= v then v = 0 end
    if v > 1 then v = 1 elseif v < -1 then v = -1 end
    out[#out+1] = string.pack("<i2", math.floor(v * 32767 + 0.5))
    if #out >= CH then f:write(table.concat(out)); out = {} end
  end
  if #out > 0 then f:write(table.concat(out)) end
  f:close(); return true
end

local function render_body()
  local ok, err = pcall(function()
    reaper.Main_OnCommand(40859, 0)               -- New project tab
    reaper.SetEditCurPos(0, false, false)
    reaper.InsertTrackAtIndex(0, false)
    local tr = reaper.GetTrack(0, 0)
    reaper.SetOnlyTrackSelected(tr)
    reaper.InsertMedia(CALIB, 0)                   -- calib-WAV som item @ t=0
    local fx = reaper.TrackFX_AddByName(tr, PLUG, false, -1)
    if fx < 0 then writedone("ERR fx=-1 (plugg ej hittad)"); return end
    reaper.TrackFX_SetEnabled(tr, fx, true)
    local _, fxname = reaper.TrackFX_GetFXName(tr, fx, "")

    -- Render via REAPER:s render-MOTOR (kör FX offline; accessor gör inte det).
    local dir  = OUT:match("^(.*)/[^/]+$")
    local base = OUT:match("/([^/]+)%.wav$")
    reaper.GetSetProjectInfo(0, "RENDER_SETTINGS", 0, true)     -- 0 = master mix
    reaper.GetSetProjectInfo(0, "RENDER_BOUNDSFLAG", 1, true)   -- 1 = entire project
    reaper.GetSetProjectInfo(0, "RENDER_CHANNELS", 2, true)
    reaper.GetSetProjectInfo(0, "RENDER_SRATE", 48000, true)
    reaper.GetSetProjectInfo(0, "RENDER_TAILFLAG", 0, true)     -- ingen extra svans
    reaper.GetSetProjectInfo_String(0, "RENDER_FILE", dir, true)
    reaper.GetSetProjectInfo_String(0, "RENDER_PATTERN", base, true)
    -- format: projektets senaste (48 kHz 24-bit WAV enl. title bar)
    reaper.Main_OnCommand(41824, 0)               -- render, auto-close dialog (synkront)
    writedone(string.format("OK fx=%d name=%s rendered=%s/%s.wav\n", fx, fxname, dir, base))
  end)
  if not ok then writedone("ERR lua="..tostring(err)) end
  pcall(function()
    reaper.Main_SaveProjectEx(0, "/tmp/bc2000_render.RPP", 0)
    reaper.Main_OnCommand(40860, 0)               -- Close current project tab
  end)
end

-- DEFERRA tills REAPER laddat klart (FX/audio-motor redo) — ~90 ticks ≈ 3 s.
-- Direkt körning i __startup.lua är för tidig → render_body() hann aldrig köra.
local ticks = 0
local function waiter()
  ticks = ticks + 1
  if ticks < 90 then reaper.defer(waiter) else render_body() end
end
reaper.defer(waiter)
LUA

rm -f "$DONE" "$OUT_WAV"
echo "==> Startar REAPER (renderar calib genom pluggen)"
"$REAPER_BIN" >/dev/null 2>&1 &

echo "==> Väntar på render (max 240s — kallstart + plugin-scan)"
for _ in $(seq 1 480); do [ -f "$DONE" ] && break; sleep 0.5; done

if [ ! -f "$DONE" ]; then
  echo "TIMEOUT — ingen render (licens-nag som blockerar startup-script?)"
  exit 1
fi
echo "==> REAPER: $(cat "$DONE")"
rm -f "$DONE"
[ -f "$OUT_WAV" ] && { echo "==> Skrev $OUT_WAV"; exit 0; } || { echo "==> Ingen WAV"; exit 1; }
