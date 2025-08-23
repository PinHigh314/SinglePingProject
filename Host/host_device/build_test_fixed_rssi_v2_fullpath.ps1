# Build script for test firmware with fixed RSSI v2
# This builds a test version that sends fixed RSSI = -55 dBm
# with improved LED control and data transmission fixes
# Using full paths to CMake and Ninja

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

# Define full paths to tools
$cmakePath = "C:\ncs\toolchains\b8b84efebd\bin\cmake.exe"
$ninjaPath = "C:\ncs\toolchains\b8b84efebd\bin\ninja.exe"

Write-Host "Using CMake: $cmakePath" -ForegroundColor Cyan
Write-Host "Using Ninja: $ninjaPath" -ForegroundColor Cyan

# Backup original files
Write-Host "Backing up original files..." -ForegroundColor Cyan
Copy-Item -Path "src/main.c" -Destination "src/main.c.backup" -Force
Copy-Item -Path "src/ble/ble_peripheral.c" -Destination "src/ble/ble_peripheral.c.backup" -Force

# Use test versions
Write-Host "Switching to test versions..." -ForegroundColor Cyan
Copy-Item -Path "src/main_test_fixed_rssi_v2.c" -Destination "src/main.c" -Force
Copy-Item -Path "src/ble/ble_peripheral_fixed.c" -Destination "src/ble/ble_peripheral.c" -Force

try {
    # Clean previous build
    Write-Host "Cleaning previous build..." -ForegroundColor Cyan
    if (Test-Path "build") {
        Remove-Item -Path "build" -Recurse -Force
    }

    # Configure with CMake using full path
    Write-Host "Configuring with CMake..." -ForegroundColor Cyan
    $configCmd = "& `"$cmakePath`" -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp"
    Write-Host "Running: $configCmd" -ForegroundColor Gray
    Invoke-Expression $configCmd

    if ($LASTEXITCODE -ne 0) {
        Write-Host "CMake configuration failed!" -ForegroundColor Red
        exit 1
    }

    # Build with Ninja using full path
    Write-Host "Building with Ninja..." -ForegroundColor Cyan
    $buildCmd = "& `"$ninjaPath`" -C build"
    Write-Host "Running: $buildCmd" -ForegroundColor Gray
    Invoke-Expression $buildCmd

    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed!" -ForegroundColor Red
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
        Write-Host "`nTest firmware v2 features:" -ForegroundColor Cyan
        Write-Host "- Fixed RSSI value: -55 dBm" -ForegroundColor White
        Write-Host "- Transmission rate: 1 Hz (every second)" -ForegroundColor White
        Write-Host "- LED3: Solid ON during streaming" -ForegroundColor White
        Write-Host "- Monitor timer ensures LED3 turns OFF when streaming stops" -ForegroundColor White
        Write-Host "- Enhanced logging for debugging data transmission" -ForegroundColor White
        Write-Host "- Fixed RSSI characteristic attribute indexing" -ForegroundColor White
        Write-Host "`nTo flash the device, use:" -ForegroundColor Cyan
        Write-Host "  nrfjprog --program $destPath --chiperase --verify -r" -ForegroundColor White
    } else {
        Write-Host "Build failed - hex file not found!" -ForegroundColor Red
        exit 1
    }
} finally {
    # Always restore original files
    Write-Host "`nRestoring original files..." -ForegroundColor Cyan
    Copy-Item -Path "src/main.c.backup" -Destination "src/main.c" -Force
    Copy-Item -Path "src/ble/ble_peripheral.c.backup" -Destination "src/ble/ble_peripheral.c" -Force
    Remove-Item -Path "src/main.c.backup" -Force
    Remove-Item -Path "src/ble/ble_peripheral.c.backup" -Force
}

Write-Host "`nDone!" -ForegroundColor Green
