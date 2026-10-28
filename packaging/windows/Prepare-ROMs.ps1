# Prepare-ROMs.ps1 - build a runtime-ready Matrimelee ROM folder from your
# own dumps. This script extracts the MAME matrim and neogeo sets and runs
# the bundled decryption tool. It does not download anything and contains no
# ROM data.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File .\Prepare-ROMs.ps1
#   powershell -ExecutionPolicy Bypass -File .\Prepare-ROMs.ps1 -MatrimZip C:\path\matrim.zip -NeoGeoZip C:\path\neogeo.zip

param(
    [string]$MatrimZip = "",
    [string]$NeoGeoZip = "",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $OutDir) { $OutDir = Join-Path $here "roms" }
$srcDir = Join-Path $here "roms-src"
$decrypt = Join-Path $here "matrimelee_decrypt.exe"

function Ask-Zip($label, $current) {
    if ($current -and (Test-Path $current)) { return (Resolve-Path $current).Path }
    while ($true) {
        $p = Read-Host "Path to $label"
        $p = $p.Trim('"').Trim()
        if (Test-Path $p) { return (Resolve-Path $p).Path }
        Write-Host "Not found: $p" -ForegroundColor Yellow
    }
}

if (-not (Test-Path $decrypt)) { throw "matrimelee_decrypt.exe is missing from this package." }

$MatrimZip = Ask-Zip "matrim.zip" $MatrimZip
$NeoGeoZip = Ask-Zip "neogeo.zip" $NeoGeoZip

New-Item -ItemType Directory -Force $OutDir | Out-Null
if (Test-Path $srcDir) { Remove-Item -Recurse -Force $srcDir }
New-Item -ItemType Directory -Force $srcDir | Out-Null

Write-Host "Extracting $MatrimZip"
tar -xf $MatrimZip -C $srcDir
if ($LASTEXITCODE -ne 0) { throw "Could not extract matrim.zip with tar" }
Write-Host "Extracting $NeoGeoZip"
tar -xf $NeoGeoZip -C $srcDir
if ($LASTEXITCODE -ne 0) { throw "Could not extract neogeo.zip with tar" }

Write-Host "Preparing decrypted runtime files (this takes a few seconds)"
& $decrypt --rom-path $srcDir --out $OutDir
if ($LASTEXITCODE -ne 0) { throw "matrimelee_decrypt failed" }

Write-Host ""
Write-Host "Done. Run:" -ForegroundColor Green
Write-Host "  .\Matrimelee.exe --rom-path `"$OutDir`""
Write-Host "The intermediate roms-src folder can be deleted; the prepared roms folder is needed."
