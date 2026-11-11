param(
    [string]$Scenario = "normal",
    [string]$BuildDir = "build/host"
)
$ErrorActionPreference = "Stop"
cmake -S . -B $BuildDir -DRATSENSE_PLATFORM=HOST_SIM -DCMAKE_BUILD_TYPE=Debug
cmake --build $BuildDir --config Debug
ctest --test-dir $BuildDir --output-on-failure -C Debug
$sim = Join-Path $BuildDir "ratsense_sim.exe"
if (-not (Test-Path $sim)) { $sim = Join-Path $BuildDir "ratsense_sim" }
& $sim --scenario $Scenario

