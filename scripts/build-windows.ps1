# Build the Windows x64 Release player and stage it for distribution.
#
# Default toolchain: the Visual Studio generator and the MSVC toolset.
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts\build-windows.ps1 `
#       -RomDir C:\path\to\unzipped\matrim+neogeo
#
# Optional:
#   -SdlDir   <dir containing sdl2-config.cmake>  (default: deps\sdl2\...\cmake)
#   -BuildDir <name>                              (default: build-windows)
#   -Parallel <n>                                 (default: 8)
#   -Test                                         run a headless smoke test
param(
    [Parameter(Mandatory = $true)][string]$RomDir,
    [string]$SdlDir = '',
    [string]$BuildDir = 'build-windows',
    [int]$Parallel = 8,
    [switch]$Test
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RomDir = (Resolve-Path $RomDir).Path
$build = Join-Path $root $BuildDir

if (-not $SdlDir) {
    $candidate = Get-ChildItem (Join-Path $root 'deps\sdl2') -Directory -ErrorAction SilentlyContinue |
        ForEach-Object { Join-Path $_.FullName 'cmake' } |
        Where-Object { Test-Path (Join-Path $_ 'sdl2-config.cmake') } |
        Select-Object -First 1
    if (-not $candidate) {
        throw 'SDL2 not found. Unpack the official SDL2 VC development package under deps\sdl2\, or pass -SdlDir.'
    }
    $SdlDir = $candidate
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio with the C++ workload was not found.' }
$vsVer = & $vswhere -latest -products * -property installationVersion
$major = ([regex]::Match("$vsVer", '^(\d+)')).Groups[1].Value
$gen = if ($major -eq '18') { 'Visual Studio 18 2026' } elseif ($major -eq '17') { 'Visual Studio 17 2022' } else { "Visual Studio $major" }

Write-Host "== configure ($gen, x64) ==" -ForegroundColor Cyan
& cmake -S $root -B $build -G $gen -A x64 `
    -DSDL2_DIR="$SdlDir" "-DCMAKE_PREFIX_PATH=$(Split-Path $SdlDir -Parent)" `
    "-DMATRIMLEE_ROM_DIR=$RomDir"
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }

Write-Host "== build Release (parallel $Parallel) ==" -ForegroundColor Cyan
& cmake --build $build --config Release --parallel $Parallel
if ($LASTEXITCODE -ne 0) { throw 'build failed' }

$exe = @((Join-Path $build 'Release\Matrimelee.exe'), (Join-Path $build 'Matrimelee.exe')) |
    Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $exe) { throw "Matrimelee.exe was not produced under $build" }
$decrypt = @((Join-Path $build 'Release\matrimelee_decrypt.exe'), (Join-Path $build 'matrimelee_decrypt.exe')) |
    Where-Object { Test-Path $_ } | Select-Object -First 1

if ($Test) {
    Write-Host "== headless verify smoke test ==" -ForegroundColor Cyan
    $decDir = Join-Path $build 'roms'
    if (-not (Test-Path (Join-Path $decDir '266-s1.s1'))) { throw "prepared ROM folder not found: $decDir" }
    $log = Join-Path $build 'smoke.log'
    & cmd.exe /c "`"$exe`" --headless --verify --rom-path `"$decDir`" --frames 1200 > `"$log`" 2>&1"
    $code = $LASTEXITCODE
    Get-Content $log | Write-Host
    if ($code -ne 0) { throw 'headless smoke test failed' }
}

$dist = Join-Path $root 'dist\windows\Matrimelee-Windows-x64'
if (Test-Path $dist) { Remove-Item -Recurse -Force $dist }
New-Item -ItemType Directory -Force $dist, (Join-Path $dist 'LICENSES'), (Join-Path $dist 'roms') | Out-Null

Copy-Item $exe $dist
if ($decrypt) { Copy-Item $decrypt $dist }
Copy-Item (Join-Path $root 'packaging\windows\README.txt') $dist
Copy-Item (Join-Path $root 'packaging\windows\Prepare-ROMs.ps1') $dist
Copy-Item (Join-Path $root 'LICENSE') $dist
Copy-Item (Join-Path $root 'LICENSE-STATUS.md') $dist
Copy-Item (Join-Path $root 'THIRD_PARTY_NOTICES.md') $dist
Copy-Item (Join-Path $root 'docs\rom-layout.md') (Join-Path $dist 'ROM-LAYOUT.md')
Copy-Item (Join-Path $root 'docs\compatibility.md') (Join-Path $dist 'COMPATIBILITY.md')
Copy-Item (Join-Path $root 'docs\testing.md') (Join-Path $dist 'TESTING.md')
Copy-Item (Join-Path $root 'neogeorecomp\LICENSE') (Join-Path $dist 'LICENSES\neogeorecomp.txt')
Copy-Item (Join-Path $root 'neogeorecomp\NOTICE') (Join-Path $dist 'LICENSES\neogeorecomp-NOTICE.txt')
Copy-Item (Join-Path $root 'neogeorecomp\LICENSES\*.txt') (Join-Path $dist 'LICENSES')
Copy-Item (Join-Path $root 'LICENSES\*.txt') (Join-Path $dist 'LICENSES')
Copy-Item (Join-Path $root 'packaging\windows\SDL2.txt') (Join-Path $dist 'LICENSES\SDL2.txt')
Set-Content -Path (Join-Path $dist 'roms\PUT-ROM-FILES-HERE.txt') -Encoding ASCII -Value @'
Put your legally obtained original ROM files in this folder, or run
Prepare-ROMs.ps1 (recommended: it extracts and decrypts them).

  matrim.zip -> 266-p1.p1 266-p2.sp2 266-c1.c1 .. 266-c8.c8
                266-m1.m1 266-v1.v1 266-v2.v2
  neogeo.zip -> sp-s2.sp1 sfix.sfix sm1.sm1 000-lo.lo

After Prepare-ROMs.ps1, the prepared folder also contains 266-s1.s1 and the
decrypted P/M1/V/C files. No ROM data is included in this package.
'@

$sdlDll = Get-ChildItem (Split-Path (Split-Path $SdlDir -Parent) -Parent) -Recurse -Filter SDL2.dll |
    Select-Object -First 1
if (-not $sdlDll) {
    $sdlDll = @((Join-Path $build 'SDL2.dll'), (Join-Path $build 'Release\SDL2.dll')) |
        Where-Object { Test-Path $_ } | ForEach-Object { Get-Item $_ } | Select-Object -First 1
}
if ($sdlDll) { Copy-Item $sdlDll.FullName $dist } else { Write-Warning 'SDL2.dll not found; the player will need it on PATH.' }

foreach ($dll in @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')) {
    $src = Join-Path $env:SystemRoot "System32\$dll"
    if (Test-Path $src) { Copy-Item $src $dist }
}

$zip = Join-Path $root 'dist\windows\Matrimelee-Windows-x64.zip'
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path $dist -DestinationPath $zip

Write-Host "wrote $dist" -ForegroundColor Green
Write-Host "wrote $zip" -ForegroundColor Green
