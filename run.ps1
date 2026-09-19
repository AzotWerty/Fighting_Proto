$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$toolRoot = 'E:\DevTools'
$cmake = Join-Path $toolRoot 'CMake\bin\cmake.exe'
$gccRoot = Join-Path $toolRoot 'MSYS2\ucrt64\bin'
$gcc = Join-Path $gccRoot 'gcc.exe'
$gxx = Join-Path $gccRoot 'g++.exe'

if (-not (Test-Path $cmake)) {
    Write-Host 'CMake was not found in E:\DevTools\CMake.' -ForegroundColor Red
    exit 1
}
if (-not (Test-Path $gxx)) {
    Write-Host 'C++ compiler was not found. Install MSYS2/UCRT64 and run this file again.' -ForegroundColor Red
    exit 1
}

$env:Path = $gccRoot + ';' + $env:Path
$buildDir = Join-Path $projectRoot 'build-ninja'

Write-Host 'Configuring build...' -ForegroundColor Cyan
& $cmake -S $projectRoot -B $buildDir -G 'Ninja' '-DCMAKE_BUILD_TYPE=Release' ('-DCMAKE_C_COMPILER=' + $gcc) ('-DCMAKE_CXX_COMPILER=' + $gxx)
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host 'Building game...' -ForegroundColor Cyan
& $cmake --build $buildDir --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$game = Join-Path $buildDir 'NeonBrawl.exe'
Write-Host 'Starting Neon Brawl...' -ForegroundColor Green
& $game
