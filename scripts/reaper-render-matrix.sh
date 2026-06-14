#!/usr/bin/env bash
# reaper-render-matrix.sh — renderar HELA funktionsmatrisen genom Germanium 2000
# Deluxe i REAPER (headless). En session, ett fall per render → renders/out_<namn>.wav.
#
# Fall: 3 hastigheter, echo, SOS, wow av/hög, multiplay g5, tape-formula BASF/Scotch,
#       HÅRD drive (input_trim+18 + sat_drive 2.0), volym låg/hög.
# Param sätts via TrackFX_SetParamNormalized (kont.) / sträng-match (diskreta val).
# Renderar via REAPER:s render-MOTOR (kör FX offline). Städar startup.lua (trap).
set -uo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RES_DIR="$HOME/Library/Application Support/REAPER"
STARTUP="$RES_DIR/Scripts/__startup.lua"
STARTUP_BAK="$RES_DIR/Scripts/__startup.lua.bc2000bak"
CALIB_FULL="$REPO_ROOT/plugin/specrig/signals/calib_full.wav"
CALIB_FR="$REPO_ROOT/plugin/specrig/signals/calib_fr.wav"
OUTDIR="$REPO_ROOT/plugin/specrig/renders"
DONE="/tmp/bc2000_matrix_done.txt"
REAPER_BIN="/Applications/REAPER.app/Contents/MacOS/REAPER"
PLUGIN_NAME="${BC2000_PLUGIN_NAME:-Germanium 2000 Deluxe}"

cleanup() {
  if [ -f "$STARTUP_BAK" ]; then mv -f "$STARTUP_BAK" "$STARTUP"; else rm -f "$STARTUP"; fi
  rm -f /tmp/bc2000_matrix.RPP
  if pgrep -x REAPER >/dev/null 2>&1; then
    osascript -e 'tell application "REAPER" to quit' >/dev/null 2>&1 || true
    for _ in $(seq 1 60); do pgrep -x REAPER >/dev/null 2>&1 || break; sleep 0.25; done
    pkill -x REAPER >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

[ -x "$REAPER_BIN" ] || { echo "REAPER saknas"; exit 1; }
[ -f "$CALIB_FULL" ] || { echo "calib_full saknas"; exit 1; }
[ -f "$CALIB_FR" ]   || { echo "calib_fr saknas"; exit 1; }

[ -f "$STARTUP" ] && mv -f "$STARTUP" "$STARTUP_BAK"
mkdir -p "$RES_DIR/Scripts"
cat > "$STARTUP" <<LUA
local FULL, FR, OUTDIR, DONE, PLUG = "$CALIB_FULL", "$CALIB_FR", "$OUTDIR", "$DONE", "$PLUGIN_NAME"
local log = {}
local function writedone(s) local f=io.open(DONE,"w"); if f then f:write(s); f:close() end end

-- kont. param: normaliserat 0..1 direkt
local function cont(tr, fx, idx, norm) reaper.TrackFX_SetParamNormalized(tr, fx, idx, norm) end
-- diskret val: prova varje steg, matcha formaterad sträng
local function disc(tr, fx, idx, n, target)
  for i=0,n-1 do
    local nv = (n<=1) and 0 or (i/(n-1))
    reaper.TrackFX_SetParamNormalized(tr, fx, idx, nv)
    local _, s = reaper.TrackFX_GetFormattedParamValue(tr, fx, idx, "")
    if tostring(s):find(target, 1, true) then return s end
  end
  return "NOMATCH:"..target
end

-- testfall: name + setup(tr,fx). Param-index från pedalboard-dump.
local CASES = {
  -- THD/S/N/sep per hastighet (calib_full)
  {name="speed_19",  inp="full", f=function(tr,fx) disc(tr,fx,0,3,"19 cm") end},
  {name="speed_95",  inp="full", f=function(tr,fx) disc(tr,fx,0,3,"9.5 cm") end},
  {name="speed_475", inp="full", f=function(tr,fx) disc(tr,fx,0,3,"4.75 cm") end},
  -- PÅLITLIG frekvensgång (stegade toner) per hastighet
  {name="fr_speed_19",  inp="fr", f=function(tr,fx) disc(tr,fx,0,3,"19 cm") end},
  {name="fr_speed_95",  inp="fr", f=function(tr,fx) disc(tr,fx,0,3,"9.5 cm") end},
  {name="fr_speed_475", inp="fr", f=function(tr,fx) disc(tr,fx,0,3,"4.75 cm") end},
  -- frekvensgång per tape-formula (speed 19)
  {name="fr_agfa",   inp="fr", f=function(tr,fx) disc(tr,fx,37,3,"Agfa") end},
  {name="fr_basf",   inp="fr", f=function(tr,fx) disc(tr,fx,37,3,"BASF") end},
  {name="fr_scotch", inp="fr", f=function(tr,fx) disc(tr,fx,37,3,"Scotch") end},
  -- features (calib_full)
  {name="echo_on",     inp="full", f=function(tr,fx) cont(tr,fx,16,1); cont(tr,fx,17,0.6); cont(tr,fx,18,0.6); cont(tr,fx,42,0.531); cont(tr,fx,43,0.5) end},
  {name="sos_on",      inp="full", f=function(tr,fx) cont(tr,fx,32,1) end},
  {name="wow_off",     inp="full", f=function(tr,fx) cont(tr,fx,15,0.0) end},
  {name="wow_high",    inp="full", f=function(tr,fx) cont(tr,fx,15,0.75) end},
  {name="multiplay_5", inp="full", f=function(tr,fx) cont(tr,fx,22,1.0) end},
  {name="drive_hot",   inp="full", f=function(tr,fx) cont(tr,fx,48,0.875); cont(tr,fx,13,1.0); cont(tr,fx,14,1.0) end},
  {name="vol_low",     inp="full", f=function(tr,fx) cont(tr,fx,10,0.3); cont(tr,fx,11,0.3) end},
  {name="vol_high",    inp="full", f=function(tr,fx) cont(tr,fx,10,1.0); cont(tr,fx,11,1.0); cont(tr,fx,49,0.625) end},
  -- NYA fall
  {name="synchroplay",  inp="full", f=function(tr,fx) cont(tr,fx,21,1) end},
  {name="bias_lo",      inp="full", f=function(tr,fx) cont(tr,fx,12,0.0); cont(tr,fx,44,0.0) end},  -- 0.5
  {name="bias_hi",      inp="full", f=function(tr,fx) cont(tr,fx,12,1.0); cont(tr,fx,44,1.0) end},  -- 1.5
  {name="print_through",inp="full", f=function(tr,fx) cont(tr,fx,38,1.0) end},                      -- 0.05
  {name="mains_hum_50", inp="full", f=function(tr,fx) cont(tr,fx,46,0.5); disc(tr,fx,47,2,"50") end},
  {name="mains_hum_60", inp="full", f=function(tr,fx) cont(tr,fx,46,0.5); disc(tr,fx,47,2,"60") end},
}

local function fsize(p) local f=io.open(p,"rb"); if not f then return -1 end local n=f:seek("end"); f:close(); return n end
local cur_minsize = 10000000   -- sätts per fall: full ~10 MB (37 s), fr ~4 MB (16 s)

-- Start render för ett fall; returnerar output-sökväg.
local function start_case(c)
  reaper.Main_OnCommand(40859, 0)                 -- new project tab
  reaper.SetEditCurPos(0, false, false)
  reaper.InsertTrackAtIndex(0, false)
  local tr = reaper.GetTrack(0, 0)
  reaper.SetOnlyTrackSelected(tr)
  local src = (c.inp == "fr") and FR or FULL
  cur_minsize = (c.inp == "fr") and 4000000 or 10000000
  reaper.InsertMedia(src, 0)
  local fx = reaper.TrackFX_AddByName(tr, PLUG, false, -1)
  if fx < 0 then return nil end
  reaper.TrackFX_SetEnabled(tr, fx, true)
  c.f(tr, fx)
  local out = OUTDIR.."/out_"..c.name..".wav"
  os.remove(out)
  reaper.GetSetProjectInfo(0, "RENDER_SETTINGS", 0, true)
  reaper.GetSetProjectInfo(0, "RENDER_BOUNDSFLAG", 1, true)
  reaper.GetSetProjectInfo(0, "RENDER_CHANNELS", 2, true)
  reaper.GetSetProjectInfo(0, "RENDER_SRATE", 48000, true)
  reaper.GetSetProjectInfo(0, "RENDER_TAILFLAG", 0, true)
  reaper.GetSetProjectInfo_String(0, "RENDER_FILE", OUTDIR, true)
  reaper.GetSetProjectInfo_String(0, "RENDER_PATTERN", "out_"..c.name, true)
  reaper.Main_OnCommand(41824, 0)                 -- render, auto-close render dialog
  return out
end

-- defer-state-machine: en render i taget, vänta in filen (yield till REAPER).
local idx, state, curout, waitc = 1, "setup", nil, 0
local function step()
  if idx > #CASES then writedone("OK "..#log.."/"..#CASES.." | "..table.concat(log,",")); return end
  local c = CASES[idx]
  if state == "setup" then
    curout = start_case(c)
    if not curout then log[#log+1]=c.name..":FX_FAIL"; idx=idx+1; reaper.defer(step); return end
    waitc = 0; state = "wait"; reaper.defer(step)
  else -- wait: pollar tills filen är fullskriven
    waitc = waitc + 1
    if fsize(curout) >= cur_minsize then
      log[#log+1]=c.name..":ok"
      idx=idx+1; state="setup"; reaper.defer(step)   -- lämna tabben öppen (ingen spara-fråga)
    elseif waitc > 4000 then                      -- ~2 min skydd per fall
      log[#log+1]=c.name..":TIMEOUT"
      idx=idx+1; state="setup"; reaper.defer(step)
    else
      reaper.defer(step)
    end
  end
end

local ticks=0
local function waiter() ticks=ticks+1; if ticks<90 then reaper.defer(waiter) else step() end end
reaper.defer(waiter)
LUA

rm -f "$DONE"
echo "==> Startar REAPER (renderar matris, $(grep -c 'name=' "$STARTUP") fall)"
"$REAPER_BIN" >/dev/null 2>&1 &
echo "==> Väntar (max 900s — 23 renders)"
for _ in $(seq 1 1800); do [ -f "$DONE" ] && break; sleep 0.5; done
[ -f "$DONE" ] || { echo "TIMEOUT"; exit 1; }
echo "==> REAPER: $(cat "$DONE")"
rm -f "$DONE"
echo "==> Renders i $OUTDIR:"; ls -1 "$OUTDIR"/out_*.wav 2>/dev/null | xargs -n1 basename 2>/dev/null
