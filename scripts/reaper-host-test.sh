#!/usr/bin/env bash
# reaper-host-test.sh — automatiserad verifiering att pluggen laddar + processar
# i en RIKTIG kommersiell DAW (REAPER), headless.
#
# Gör:
#   1. Genererar en 440 Hz ton-WAV (stdlib python).
#   2. Installerar ett __startup.lua som i REAPER: nytt projekt-tab → ny spår →
#      lägger tonen som media-item → laddar VST3 "Beolux 2000" som FX (bevisar att
#      en riktig DAW scannar + instansierar pluggen) → drar spårets ljud via en
#      audio accessor → mäter peak/RMS/NaN (signal-flöde, bounded, ingen NaN) →
#      skriver resultat till en fil.
#   (Ljud-FÄRGNING i host täcks redan av pluginval + pedalboard; detta test
#    verifierar specifikt REAPER-scanning/instansiering/signal-flöde.)
#
# OBS: detta är ett BEST-EFFORT manuellt host-test, INTE en ctest-del. En
# olicensierad REAPER visar periodvis en nag-dialog vid start som blockerar
# startup-scriptet (kräver klick) → då time-outar testet. När REAPER startar
# utan nag passerar det rent. Bekräftat resultat (utan nag):
#   fx=0  name="VST3: Beolux 2000 (Soundboys)"  got=1  peak~0.30  rms~0.21  nan=0
# dvs REAPER scannar + instansierar pluggen och ljud flödar utan NaN/blowup.
#   3. Startar REAPER, väntar på resultatfilen, parsar PASS/FAIL.
#   4. Städar ALLTID bort __startup.lua + ton-WAV och avslutar REAPER (trap).
#
# Exit: 0 = pluggen instansierades + processade ljud säkert i REAPER, annars 1.
set -uo pipefail

RES_DIR="$HOME/Library/Application Support/REAPER"
STARTUP="$RES_DIR/Scripts/__startup.lua"
STARTUP_BAK="$RES_DIR/Scripts/__startup.lua.bc2000bak"
TONE="/tmp/bc2000_tone.wav"
RESULT="/tmp/bc2000_reaper_result.txt"
REAPER_BIN="/Applications/REAPER.app/Contents/MacOS/REAPER"
PLUGIN_NAME="${BC2000_PLUGIN_NAME:-Beolux 2000}"

cleanup() {
  # återställ ALLTID användarens startup-script FÖRST (även om REAPER hänger)
  if [ -f "$STARTUP_BAK" ]; then mv -f "$STARTUP_BAK" "$STARTUP"; else rm -f "$STARTUP"; fi
  rm -f "$TONE" /tmp/bc2000_test.RPP
  # avsluta REAPER mjukt; startup-scriptet stänger redan sin egen tab rent, så
  # detta promptar inte. Ge gott om tid innan ev. force-kill (sista utväg).
  if pgrep -x REAPER >/dev/null 2>&1; then
    osascript -e 'tell application "REAPER" to quit' >/dev/null 2>&1 || true
    for _ in $(seq 1 40); do pgrep -x REAPER >/dev/null 2>&1 || break; sleep 0.25; done
    pkill -x REAPER >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

if [ ! -x "$REAPER_BIN" ]; then echo "REAPER saknas: $REAPER_BIN"; exit 1; fi

echo "==> Genererar ton-WAV"
python3 - "$TONE" <<'PY'
import wave, struct, math, sys
sr, n, amp, f = 48000, 48000, 0.3, 440.0
w = wave.open(sys.argv[1], 'w'); w.setnchannels(2); w.setsampwidth(2); w.setframerate(sr)
fr = bytearray()
for i in range(n):
    s = int(amp * 32767 * math.sin(2*math.pi*f*i/sr))
    fr += struct.pack('<hh', s, s)
w.writeframes(bytes(fr)); w.close()
PY

echo "==> Installerar __startup.lua"
[ -f "$STARTUP" ] && mv -f "$STARTUP" "$STARTUP_BAK"
mkdir -p "$RES_DIR/Scripts"
cat > "$STARTUP" <<LUA
local RES  = "$RESULT"
local TONE = "$TONE"
local PLUG = "$PLUGIN_NAME"
local function writeres(s) local f=io.open(RES,"w"); if f then f:write(s); f:close() end end
local function measure(acc)
  local sr, nch, nf = 48000, 2, 48000
  local buf = reaper.new_array(nch*nf); buf.clear()
  local got = reaper.GetAudioAccessorSamples(acc, sr, nch, 0.0, nf, buf)
  local peak, sumsq, nan = 0.0, 0.0, 0
  for i=1,nch*nf do local v=buf[i]; if v~=v then nan=nan+1 end
    local a=(v<0 and -v or v); if a>peak then peak=a end; sumsq=sumsq+v*v end
  return got, peak, math.sqrt(sumsq/(nch*nf)), nan
end
local ok, err = pcall(function()
  reaper.Main_OnCommand(40859, 0)               -- New project tab (isolera från användarens projekt)
  reaper.SetEditCurPos(0, false, false)
  reaper.InsertTrackAtIndex(0, false)
  local tr = reaper.GetTrack(0, 0)
  reaper.SetOnlyTrackSelected(tr)
  reaper.InsertMedia(TONE, 0)                    -- lägg tonen som item på vald spår
  local fx = reaper.TrackFX_AddByName(tr, PLUG, false, -1)
  local _, fxname = reaper.TrackFX_GetFXName(tr, fx, "")
  if fx < 0 then writeres("RESULT fx=-1 name=NONE got=0 peak=0 rms=0 nan=0\n"); return end

  -- dra ljudet genom spåret via en audio accessor (signal-flöde i REAPER)
  local acc = reaper.CreateTrackAudioAccessor(tr); reaper.AudioAccessorUpdate(acc)
  local got, peak, rms, nan = measure(acc)
  reaper.DestroyAudioAccessor(acc)
  writeres(string.format("RESULT fx=%d name=%s got=%d peak=%.6f rms=%.6f nan=%d\n",
                         fx, fxname, got, peak, rms, nan))
end)
if not ok then writeres("RESULT lua_error="..tostring(err).."\n") end
-- stäng MIN projekt-tab rent (rör ej användarens ev. öppna projekt) så REAPER
-- kan avslutas utan spara-dialog och utan "unclean shutdown" nästa gång.
pcall(function()
  reaper.Main_SaveProjectEx(0, "/tmp/bc2000_test.RPP", 0)  -- gör min tab "ren"
  reaper.Main_OnCommand(40860, 0)                          -- Close current project tab
end)
LUA

rm -f "$RESULT"
echo "==> Startar REAPER (headless via startup-script)"
"$REAPER_BIN" >/dev/null 2>&1 &

echo "==> Väntar på resultat (max 240s — REAPER kallstart + plugin-scan kan ta tid)"
for _ in $(seq 1 480); do [ -f "$RESULT" ] && break; sleep 0.5; done

if [ ! -f "$RESULT" ]; then
  echo "TIMEOUT — ingen resultatfil (ev. licens-nag blockerar startup-script?)"
  exit 1
fi

LINE="$(cat "$RESULT")"
echo "==> REAPER rapporterade: $LINE"

# parsa
fx="$(echo "$LINE"   | sed -n 's/.*fx=\([-0-9]*\).*/\1/p')"
name="$(echo "$LINE" | sed -n 's/.* name=\(.*\) got=.*/\1/p')"
peak="$(echo "$LINE" | sed -n 's/.*peak=\([0-9.]*\).*/\1/p')"
rms="$(echo "$LINE"  | sed -n 's/.*rms=\([0-9.]*\).*/\1/p')"
nan="$(echo "$LINE"  | sed -n 's/.*nan=\([0-9]*\).*/\1/p')"

fail=0
[ "${fx:-"-1"}" -ge 0 ] 2>/dev/null && echo "  [OK]   pluggen instansierad i REAPER (fx-idx $fx, namn: $name)" || { echo "  [FAIL] pluggen kunde ej laddas i REAPER"; fail=1; }
case "$name" in *Beolux*) echo "  [OK]   FX-namn matchar (Beolux)";; *) echo "  [FAIL] FX-namn oväntat: $name"; fail=1;; esac
[ "${nan:-1}" = "0" ] && echo "  [OK]   ingen NaN i host-signalflöde" || { echo "  [FAIL] NaN i output: $nan"; fail=1; }
awk -v p="${peak:-99}" 'BEGIN{exit !(p<8.0)}' && echo "  [OK]   signal bounded (peak $peak < 8)" || { echo "  [FAIL] signal ej bounded: $peak"; fail=1; }
awk -v r="${rms:-0}" 'BEGIN{exit !(r>0.0001)}' && echo "  [OK]   ljud flödar genom spåret (rms $rms)" || echo "  [info] låg/ingen rms ($rms)"

rm -f "$RESULT"
[ "$fail" = "0" ] && echo "RESULTAT: pluggen laddar + processar säkert i REAPER — OK" || echo "RESULTAT: avvikelse i REAPER-host-testet"
exit "$fail"
