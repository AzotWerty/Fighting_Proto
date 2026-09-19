$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildScript = Join-Path $projectRoot 'run.ps1'
$buildDir = Join-Path $projectRoot 'build-ninja'
$game = Join-Path $buildDir 'NeonBrawl.exe'
$portableDir = Join-Path $projectRoot 'portable'

& powershell -NoProfile -ExecutionPolicy Bypass -File $buildScript
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if (Test-Path $portableDir) {
    Remove-Item $portableDir -Recurse -Force
}
New-Item $portableDir -ItemType Directory | Out-Null
Copy-Item $game (Join-Path $portableDir 'NeonBrawl.exe')
$assets = Join-Path $projectRoot 'assets'
if (Test-Path $assets) {
    Copy-Item $assets (Join-Path $portableDir 'assets') -Recurse -Force
}

@'
@echo off
cd /d "%~dp0"
start "Neon Brawl" "%~dp0NeonBrawl.exe"
'@ | Set-Content (Join-Path $portableDir 'NeonBrawl.bat') -Encoding ASCII

@'
NEON BRAWL - PORTABLE BUILD

Copy this entire folder to any Windows computer and double-click NeonBrawl.bat.
No CMake, compiler, MSYS2, source code, or internet connection is needed.

Controls:
A / D or arrow keys - move
J - punch
K - heavy kick
Enter / Space - confirm
Main menu: Up / Down - choose item
Settings: Up / Down - choose field, Left / Right - change value
Esc - menu
Default display mode: fullscreen at the current monitor resolution

Assets:
assets/menu/background.png - main menu background
assets/arenas/<name>/background.png - arena background
assets/arenas/<name>/ground.png - arena ground, anchored to the bottom
Recommended ground size: 1280x80
'@ | Set-Content (Join-Path $portableDir 'README.txt') -Encoding ASCII

Write-Host "Portable package created: $portableDir" -ForegroundColor Green
