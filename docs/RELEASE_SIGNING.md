# Release-signering & notarisering

Detta beskriver hur en **kommersiell** Beolux 2000-release signeras så att den
laddas utan varningar på slutanvändares maskiner. Skilt från
`scripts/package-dist.sh` (ad-hoc-signering för utvecklingsdistribution till en
kompis).

> Certifikat och lösenord finns **inte** i repot och ska aldrig committas. De
> matas in som miljövariabler lokalt eller som GitHub Actions-secrets.

---

## macOS — Developer ID + notarisering

**Krav:** Apple Developer Program-medlemskap ($99/år) → ger två certifikat:
- *Developer ID Application* (signerar plugin-bundlarna)
- *Developer ID Installer* (signerar .pkg:n)

**Engångsuppsättning av notarytool-profil:**
```sh
xcrun notarytool store-credentials AC_NOTARY_PROFILE \
  --apple-id you@soundboys.com --team-id TEAMID \
  --password <app-specific-password>   # skapas på appleid.apple.com
```

**Bygg + signera + notarisera:**
```sh
cmake --build plugin/juce/build --target BC2000DL_All -j8
DEV_ID_APP="Developer ID Application: Soundboys ApS (TEAMID)" \
DEV_ID_INSTALLER="Developer ID Installer: Soundboys ApS (TEAMID)" \
AC_NOTARY_PROFILE="AC_NOTARY_PROFILE" \
scripts/release-mac-notarize.sh
```
Resultat: `output/pkg/Beolux_2000_<version>_macOS.pkg` — notariserad + stapled,
verifierad med `spctl --assess --type install`.

Scriptet använder hardened runtime + secure timestamp (krav för notarisering)
och bygger en distributions-pkg som installerar VST3 + AU till
`/Library/Audio/Plug-Ins/{VST3,Components}`.

---

## Windows — Authenticode

**Krav:** ett kodsigneringscertifikat. **OV** fungerar men SmartScreen kräver
inarbetat rykte innan varningar försvinner; **EV** ger omedelbart förtroende
(rekommenderas för kommersiell release). Köps från DigiCert/Sectigo m.fl.

**Signera den byggda VST3:n:**
```powershell
pwsh scripts/release-windows-sign.ps1 `
    -Vst3 "plugin\juce\build\BC2000DL_artefacts\Release\VST3\Beolux 2000.vst3" `
    -PfxPath C:\certs\soundboys.pfx -PfxPassword $env:PFX_PW
```
eller via cert i Windows-arkivet: `-Thumbprint "ABCD…"`.

Scriptet signerar `.vst3\Contents\x86_64-win\Beolux 2000.vst3` (SHA256 +
RFC3161-tidsstämpel) och verifierar med `signtool verify /pa`.

---

## GitHub Actions-secrets (för signerade release-artefakter i CI)

Lägg under **Settings → Secrets and variables → Actions**. CI-jobben i
`.github/workflows/ci.yml` bygger osignerat; en separat release-workflow kan
plocka dessa när den finns:

| Secret | Plattform | Innehåll |
|---|---|---|
| `MAC_DEV_ID_APP_P12` | mac | base64 av Developer ID Application-.p12 |
| `MAC_DEV_ID_INSTALLER_P12` | mac | base64 av Developer ID Installer-.p12 |
| `MAC_P12_PASSWORD` | mac | lösenord till .p12-filerna |
| `MAC_NOTARY_APPLE_ID` / `MAC_NOTARY_TEAM_ID` / `MAC_NOTARY_PW` | mac | notarytool-credentials |
| `WIN_PFX_BASE64` | win | base64 av kodsignerings-.pfx |
| `WIN_PFX_PASSWORD` | win | lösenord till .pfx |

> ⚠️ AAX (Pro Tools) kräver **separat** Avid-avtal + PACE/iLok-signering och
> täcks inte här. Aktiveras endast om Pro Tools-marknaden prioriteras (Fas 4-tillägg).

---

## Status

- [x] Mac notariserings-script (`release-mac-notarize.sh`) — körbart, kräver cert
- [x] Windows Authenticode-script (`release-windows-sign.ps1`) — körbart, kräver cert
- [ ] Certifikat anskaffade (Apple Developer + Windows codesign-cert)
- [ ] Release-workflow i Actions som signerar artefakterna automatiskt
- [ ] AAX/iLok — endast om Pro Tools prioriteras
