# Build script for Host Device Test Firmware v8 with Real BLE Central
Write-Host "Building Host Device Test Firmware v8 - Dual Connection with Mipe" -ForegroundColor Cyan
Write-Host ""

# Set environment variables
Write-Host "Setting up environment variables..." -ForegroundColor Yellow
$env:ZEPHYR_BASE = "C:/ncs/v3.1.0/zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:/ncs/toolchains/b8b84efebd/opt/zephyr-sdk"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"

# Add toolchain to PATH
$toolchainPath = "C:\ncs\toolchains\b8b84efebd\opt\bin;C:\ncs\toolchains\b8b84efebd\opt\bin\Scripts"
if ($env:PATH -notlike "*$toolchainPath*") {
    $env:PATH = "$toolchainPath;$env:PATH"
}

# Navigate to host_device directory
Set-Location "C:\Development\SinglePingProject\Host\host_device"

# Clean build directory
Write-Host "Cleaning build directory..." -ForegroundColor Yellow
if (Test-Path "build") {
    Remove-Item -Recurse -Force "build"
}

# Backup current files
Write-Host "Backing up current files..." -ForegroundColor Yellow
if (Test-Path "src/main.c") {
    Copy-Item "src/main.c" "src/main.c.backup" -Force
}
if (Test-Path "src/ble/ble_central.c") {
    Copy-Item "src/ble/ble_central.c" "src/ble/ble_central.c.backup" -Force
}
if (Test-Path "src/ble/ble_peripheral.c") {
    Copy-Item "src/ble/ble_peripheral.c" "src/ble/ble_peripheral.c.backup" -Force
}
if (Test-Path "src/ble/ble_peripheral.h") {
    Copy-Item "src/ble/ble_peripheral.h" "src/ble/ble_peripheral.h.backup" -Force
}

# Install test version files
Write-Host "Installing test version v8 with REAL BLE Central..." -ForegroundColor Yellow
Copy-Item "src/main_test_fixed_rssi_v8.c" "src/main.c" -Force
Copy-Item "src/ble/ble_central_real.c" "src/ble/ble_central.c" -Force
Copy-Item "src/ble/ble_peripheral_v8.c" "src/ble/ble_peripheral.c" -Force
Copy-Item "src/ble/ble_peripheral_v8.h" "src/ble/ble_peripheral.h" -Force

# Configure with CMake
Write-Host "Configuring with CMake..." -ForegroundColor Yellow
$configCmd = "cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp"
Invoke-Expression $configCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    # Restore original files
    if (Test-Path "src/main.c.backup") {
        Copy-Item "src/main.c.backup" "src/main.c" -Force
        Remove-Item "src/main.c.backup"
    }
    if (Test-Path "src/ble/ble_central.c.backup") {
        Copy-Item "src/ble/ble_central.c.backup" "src/ble/ble_central.c" -Force
        Remove-Item "src/ble/ble_central.c.backup"
    }
    exit 1
}

# Build with Ninja
Write-Host "Building with Ninja..." -ForegroundColor Yellow
$buildCmd = "ninja -C build"
Invoke-Expression $buildCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    # Restore original files
    if (Test-Path "src/main.c.backup") {
        Copy-Item "src/main.c.backup" "src/main.c" -Force
        Remove-Item "src/main.c.backup"
    }
    if (Test-Path "src/ble/ble_central.c.backup") {
        Copy-Item "src/ble/ble_central.c.backup" "src/ble/ble_central.c" -Force
        Remove-Item "src/ble/ble_central.c.backup"
    }
    exit 1
}

Write-Host ""
Write-Host "Build completed successfully!" -ForegroundColor Green

# Copy hex file to compiled_code
$date = Get-Date -Format 'yyyyMMdd'
$hexSource = "build/zephyr/zephyr.hex"
$hexDest = "../../compiled_code/host_device_test_dual_connection_mipe_v8_${date}_rev021.hex"

if (Test-Path $hexSource) {
    Copy-Item $hexSource $hexDest -Force
    Write-Host "Firmware copied to: $hexDest" -ForegroundColor Cyan
    
    # Display file info
    $fileInfo = Get-Item $hexDest
    Write-Host "File size: $([math]::Round($fileInfo.Length / 1KB, 2)) KB" -ForegroundColor White
}

# Restore original files
Write-Host "Restoring original files..." -ForegroundColor Yellow
if (Test-Path "src/main.c.backup") {
    Copy-Item "src/main.c.backup" "src/main.c" -Force
    Remove-Item "src/main.c.backup"
}
if (Test-Path "src/ble/ble_central.c.backup") {
    Copy-Item "src/ble/ble_central.c.backup" "src/ble/ble_central.c" -Force
    Remove-Item "src/ble/ble_central.c.backup"
}
if (Test-Path "src/ble/ble_peripheral.c.backup") {
    Copy-Item "src/ble/ble_peripheral.c.backup" "src/ble/ble_peripheral.c" -Force
    Remove-Item "src/ble/ble_peripheral.c.backup"
}
if (Test-Path "src/ble/ble_peripheral.h.backup") {
    Copy-Item "src/ble/ble_peripheral.h.backup" "src/ble/ble_peripheral.h" -Force
    Remove-Item "src/ble/ble_peripheral.h.backup"
}

Write-Host ""
Write-Host "===== Build Complete =====" -ForegroundColor Green
Write-Host "Version: v8 - Dual Connection with Real Mipe" -ForegroundColor Yellow
Write-Host "Features:" -ForegroundColor Yellow
Write-Host "  - Alternating RSSI transmission" -ForegroundColor White
Write-Host "  - Real BLE connection to Mipe device" -ForegroundColor White
Write-Host "  - LED1 rapid flash (100ms) when dual connected" -ForegroundColor White
Write-Host "Output: $hexDest" -ForegroundColor Cyan
Write-Host ""
Write-Host "To flash: nrfjprog --program $hexDest --chiperase --verify -r" -ForegroundColor Cyan
