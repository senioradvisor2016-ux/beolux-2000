# release-windows-sign.ps1 — Authenticode-signera Windows VST3:n.
#
# Authenticode-signering krävs för att Windows SmartScreen inte ska varna
# slutanvändare. Kräver ett kodsigneringscertifikat (helst EV/OV) — finns
# INTE i repot, pekas ut via parametrar/env.
#
# FÖRKRAV:
#   - signtool.exe i PATH (Windows SDK)
#   - ett .pfx-certifikat + lösenord, ELLER ett cert i Windows-certarkivet
#
# Användning (pfx-fil):
#   pwsh scripts/release-windows-sign.ps1 `
#       -Vst3 "plugin\juce\build\BC2000DL_artefacts\Release\VST3\Beolux 2000.vst3" `
#       -PfxPath C:\certs\soundboys.pfx -PfxPassword $env:PFX_PW
#
# Användning (cert i arkivet via thumbprint):
#   pwsh scripts/release-windows-sign.ps1 -Vst3 "...\Beolux 2000.vst3" `
#       -Thumbprint "ABCD1234..."

param(
    [Parameter(Mandatory=$true)] [string]$Vst3,
    [string]$PfxPath,
    [string]$PfxPassword,
    [string]$Thumbprint,
    [string]$TimestampUrl = "http://timestamp.digicert.com"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $Vst3)) { throw "Hittar inte VST3: $Vst3" }
if (-not $PfxPath -and -not $Thumbprint) {
    Write-Host "Detta script kräver ett kodsigneringscertifikat." -ForegroundColor Yellow
    Write-Host "Ange antingen -PfxPath (+ -PfxPassword) eller -Thumbprint." -ForegroundColor Yellow
    Write-Host "Se docs/RELEASE_SIGNING.md. Avbryter."
    exit 1
}

$signtool = (Get-Command signtool.exe -ErrorAction SilentlyContinue)
if (-not $signtool) {
    throw "signtool.exe saknas i PATH — installera Windows SDK."
}

# VST3 på Windows är en mapp-bundle; den signerbara binären är .vst3\Contents\x86_64-win\<namn>.vst3
$dll = Get-ChildItem -Path $Vst3 -Recurse -Filter "*.vst3" |
       Where-Object { $_.FullName -match "x86_64-win" } | Select-Object -First 1
if (-not $dll) { throw "Hittar ingen .vst3-binär under $Vst3\Contents\x86_64-win\" }

$args = @("sign", "/fd", "SHA256", "/tr", $TimestampUrl, "/td", "SHA256")
if ($PfxPath) {
    $args += @("/f", $PfxPath)
    if ($PfxPassword) { $args += @("/p", $PfxPassword) }
} else {
    $args += @("/sha1", $Thumbprint)
}
$args += $dll.FullName

Write-Host "==> Signerar $($dll.FullName)"
& signtool.exe @args

Write-Host "==> Verifierar"
& signtool.exe verify /pa /v $dll.FullName

Write-Host "`n✓ Klar: Authenticode-signerad" -ForegroundColor Green
