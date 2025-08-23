# Build script for test firmware with fixed RSSI v2
# This builds using the current source files which already have v2 fixes

$ErrorActionPreference = "Stop"

# Get the current date and revision number
$date = Get-Date -Format "yyyyMMdd"
$revision = "015"  # v2 of test firmware
$outputName = "host_device_test_fixed_rssi_v2_${date}_rev${revision}"

Write-Host "Building Host Device Test Firmware v2 (Fixed RSSI = -55 dBm)..." -ForegroundColor Green
Write-Host "Output: $outputName.hex" -ForegroundColor Yellow
Write-Host ""
Write-Host "Using current source files with v2 fixes:" -ForegroundColor Cyan
Write-Host "- LED3 monitor timer to ensure turn-off" -ForegroundColor White
Write-Host "- Fixed RSSI characteristic attribute indexing" -ForegroundColor White
Write-Host "- Force-enable notifications on stream start" -ForegroundColor White
Write-Host ""

# Set environment variables for nRF Connect SDK v3.1.0
Write-Host "Setting up environment variables..." -ForegroundColor Yellow
$env:ZEPHYR_BASE = "C:/ncs/v3.1.0/zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:/ncs/toolchains/b8b84efebd/opt/zephyr-sdk"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"

# Add toolchain to PATH
$toolchainPath = "C:\ncs\toolchains\b8b84efebd\opt\bin;C:\ncs\toolchains\b8b84efebd\opt\bin\Scripts"
if ($env:PATH -notlike "*$toolchainPath*") {
    $env:PATH = "$toolchainPath;$env:PATH"
}

# Clean build directory
Write-Host "Cleaning build directory..." -ForegroundColor Cyan
if (Test-Path "build") {
    Remove-Item -Path "build" -Recurse -Force
}

# Build with the current source files
Write-Host "Configuring with CMake..." -ForegroundColor Cyan
$configCmd = "cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp"
Write-Host "Running: $configCmd" -ForegroundColor Gray
$configResult = Invoke-Expression $configCmd 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    Write-Host $configResult
    exit 1
}

Write-Host "CMake configuration successful!" -ForegroundColor Green

# Build with Ninja
Write-Host "Building with Ninja..." -ForegroundColor Cyan
$buildCmd = "ninja -C build"
Write-Host "Running: $buildCmd" -ForegroundColor Gray
$buildResult = Invoke-Expression $buildCmd 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    Write-Host $buildResult
    exit 1
}

# Check if hex file was created
$hexPath = "build/zephyr/zephyr.hex"
if (Test-Path $hexPath) {
    # Copy to compiled_code with descriptive name
    $destPath = "../../compiled_code/$outputName.hex"
    Copy-Item -Path $hexPath -Destination $destPath -Force
    
    # Get file size
    $fileSize = (Get-Item $destPath).Length / 1KB
    
    Write-Host "`nBuild successful!" -ForegroundColor Green
    Write-Host "Firmware saved to: $destPath" -ForegroundColor Yellow
    Write-Host "File size: $([math]::Round($fileSize, 2)) KB" -ForegroundColor Yellow
    Write-Host "`nTest Firmware v2 Features:" -ForegroundColor Cyan
    Write-Host "- Fixed RSSI value: -55 dBm" -ForegroundColor White
    Write-Host "- Transmission rate: 1 second" -ForegroundColor White
    Write-Host "- LED3: Solid ON during streaming" -ForegroundColor White
    Write-Host "- Monitor timer ensures LED3 turns OFF when streaming stops" -ForegroundColor White
    Write-Host "- Corrected RSSI characteristic attribute indexing" -ForegroundColor White
    Write-Host "- Force-enables notifications on stream start" -ForegroundColor White
    Write-Host "`nThis version fixes the issues from rev014:" -ForegroundColor Green
    Write-Host "✓ LED3 now turns OFF when streaming stops" -ForegroundColor Green
    Write-Host "✓ RSSI data transmission should work correctly" -ForegroundColor Green
    Write-Host "`nTo flash the device, use:" -ForegroundColor Cyan
    Write-Host "  nrfjprog --program $destPath --chiperase --verify -r" -ForegroundColor White
} else {
    Write-Host "Build failed - hex file not found!" -ForegroundColor Red
    exit 1
}

Write-Host "`nDone!" -ForegroundColor Green
