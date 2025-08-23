# Build script for test firmware with fixed RSSI v2
# This builds a test version that sends fixed RSSI = -55 dBm
# Uses patches instead of full file replacement

$ErrorActionPreference = "Stop"

# Get the current date and revision number
$date = Get-Date -Format "yyyyMMdd"
$revision = "015"  # v2 of test firmware
$outputName = "host_device_test_fixed_rssi_v2_${date}_rev${revision}"

Write-Host "Building Host Device Test Firmware v2 (Fixed RSSI = -55 dBm)..." -ForegroundColor Green
Write-Host "Output: $outputName.hex" -ForegroundColor Yellow

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

# Build with the current source files (already contain the fixes from rev014)
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
    Write-Host "`nThis is the same firmware as rev014 but rebuilt with current SDK" -ForegroundColor Cyan
    Write-Host "Features:" -ForegroundColor Cyan
    Write-Host "- Fixed RSSI value: -55 dBm" -ForegroundColor White
    Write-Host "- Transmission rate: 2 seconds (rate limited)" -ForegroundColor White
    Write-Host "- LED3: Toggles every 5 packets during streaming" -ForegroundColor White
    Write-Host "`nKnown issues to fix in next version:" -ForegroundColor Yellow
    Write-Host "- LED3 doesn't turn OFF when streaming stops" -ForegroundColor White
    Write-Host "- RSSI data may not be transmitting correctly" -ForegroundColor White
    Write-Host "`nTo flash the device, use:" -ForegroundColor Cyan
    Write-Host "  nrfjprog --program $destPath --chiperase --verify -r" -ForegroundColor White
} else {
    Write-Host "Build failed - hex file not found!" -ForegroundColor Red
    exit 1
}

Write-Host "`nDone!" -ForegroundColor Green
