#!/usr/bin/env bash
# reaper-sos-multipass.sh — Sound-on-Sound multi-pass-test i REAPER.
# 4 pass: pass1 = calib_full → out_sos_p1; passN = out_sos_p(N-1) → out_sos_pN.
# SOS på i varje pass. Mät sos_sustain-RMS per pass → lager-uppbyggnad.
set -uo pipefail
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RES_DIR="$HOME/Library/Application Support/REAPER"
STARTUP="$RES_DIR/Scripts/__startup.lua"; STARTUP_BAK="$STARTUP.bc2000bak"
FULL="$REPO_ROOT/plugin/specrig/signals/calib_full.wav"
OUTDIR="$REPO_ROOT/plugin/specrig/renders"
DONE="/tmp/bc2000_sos_done.txt"
REAPER_BIN="/Applications/REAPER.app/Contents/MacOS/REAPER"
PLUG="${BC2000_PLUGIN_NAME:-Germanium 2000 Deluxe}"
PASSES="${1:-4}"

cleanup() {
  if [ -f "$STARTUP_BAK" ]; then mv -f "$STARTUP_BAK" "$STARTUP"; else rm -f "$STARTUP"; fi
  if pgrep -x REAPER >/dev/null 2>&1; then
    osascript -e 'tell application "REAPER" to quit' >/dev/null 2>&1 || true
    for _ in $(seq 1 60); do pgrep -x REAPER >/dev/null 2>&1 || break; sleep 0.25; done
    pkill -x REAPER >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT
[ -x "$REAPER_BIN" ] || { echo "REAPER saknas"; exit 1; }

[ -f "$STARTUP" ] && mv -f "$STARTUP" "$STARTUP_BAK"
mkdir -p "$RES_DIR/Scripts"
cat > "$STARTUP" <<LUA
local FULL,OUTDIR,DONE,PLUG,N = "$FULL","$OUTDIR","$DONE","$PLUG",$PASSES
local log={}
local function writedone(s) local f=io.open(DONE,"w"); if f then f:write(s);f:close() end end
local function fsize(p) local f=io.open(p,"rb"); if not f then return -1 end local n=f:seek("end");f:close();return n end

local function start_pass(n)
  local inp = (n==1) and FULL or (OUTDIR.."/out_sos_p"..(n-1)..".wav")
  reaper.Main_OnCommand(40859,0); reaper.SetEditCurPos(0,false,false)
  reaper.InsertTrackAtIndex(0,false); local tr=reaper.GetTrack(0,0); reaper.SetOnlyTrackSelected(tr)
  reaper.InsertMedia(inp,0)
  local fx=reaper.TrackFX_AddByName(tr,PLUG,false,-1)
  if fx<0 then return nil end
  reaper.TrackFX_SetEnabled(tr,fx,true)
  reaper.TrackFX_SetParamNormalized(tr,fx,32,1)   -- sound_on_sound ON
  local out=OUTDIR.."/out_sos_p"..n..".wav"; os.remove(out)
  reaper.GetSetProjectInfo(0,"RENDER_SETTINGS",0,true)
  reaper.GetSetProjectInfo(0,"RENDER_BOUNDSFLAG",1,true)
  reaper.GetSetProjectInfo(0,"RENDER_CHANNELS",2,true)
  reaper.GetSetProjectInfo(0,"RENDER_SRATE",48000,true)
  reaper.GetSetProjectInfo(0,"RENDER_TAILFLAG",0,true)
  reaper.GetSetProjectInfo_String(0,"RENDER_FILE",OUTDIR,true)
  reaper.GetSetProjectInfo_String(0,"RENDER_PATTERN","out_sos_p"..n,true)
  reaper.Main_OnCommand(41824,0)
  return out
end

local n,state,curout,wc = 1,"setup",nil,0
local function step()
  if n>N then writedone("OK "..#log.."/"..N.." | "..table.concat(log,",")); return end
  if state=="setup" then
    curout=start_pass(n)
    if not curout then log[#log+1]="p"..n..":FX_FAIL"; n=n+1; reaper.defer(step); return end
    wc=0; state="wait"; reaper.defer(step)
  else
    wc=wc+1
    if fsize(curout)>=10000000 then log[#log+1]="p"..n..":ok"; n=n+1; state="setup"; reaper.defer(step)
    elseif wc>4000 then log[#log+1]="p"..n..":TIMEOUT"; n=n+1; state="setup"; reaper.defer(step)
    else reaper.defer(step) end
  end
end
local t=0
local function w() t=t+1; if t<90 then reaper.defer(w) else step() end end
reaper.defer(w)
LUA

rm -f "$DONE"
echo "==> Startar REAPER (SOS $PASSES pass)"
"$REAPER_BIN" >/dev/null 2>&1 &
for _ in $(seq 1 1200); do [ -f "$DONE" ] && break; sleep 0.5; done
[ -f "$DONE" ] || { echo "TIMEOUT"; exit 1; }
echo "==> REAPER: $(cat "$DONE")"; rm -f "$DONE"
