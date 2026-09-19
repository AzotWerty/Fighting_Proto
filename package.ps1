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

@'
@echo off
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
'@ | Set-Content (Join-Path $portableDir 'README.txt') -Encoding ASCII

Write-Host "Portable package created: $portableDir" -ForegroundColor Green
