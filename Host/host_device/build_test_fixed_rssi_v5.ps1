# Build script for Host Device Test Firmware v5 (Fixed LED2 and Streaming)
# This version fixes LED2 lighting up and properly triggers streaming

Write-Host "Building Host Device Test Firmware v5 (Fixed LED2 & Streaming)..." -ForegroundColor Cyan
Write-Host "Output: host_device_test_fixed_led_v5_$(Get-Date -Format 'yyyyMMdd')_rev018.hex" -ForegroundColor Yellow
Write-Host ""
Write-Host "Fixes in v5:" -ForegroundColor Green
Write-Host "- LED2 no longer lights up on MotoApp connection" -ForegroundColor White
Write-Host "- Streaming properly triggered by control command" -ForegroundColor White
Write-Host "- RSSI generation starts only when streaming starts" -ForegroundColor White
Write-Host "- LED3 toggles to show data activity" -ForegroundColor White
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

# Backup current main.c
Write-Host "Backing up current main.c..." -ForegroundColor Yellow
if (Test-Path "src/main.c") {
    Copy-Item "src/main.c" "src/main.c.backup_v5" -Force
}

# Install v5 test version
Write-Host "Installing v5 test version..." -ForegroundColor Yellow
Copy-Item "src/main_test_fixed_rssi_v5.c" "src/main.c" -Force

# Backup and replace ble_central.c with test version
Write-Host "Installing test BLE central..." -ForegroundColor Yellow
if (Test-Path "src/ble/ble_central.c") {
    Copy-Item "src/ble/ble_central.c" "src/ble/ble_central.c.backup_v5" -Force
}
Copy-Item "src/ble/ble_central_test.c" "src/ble/ble_central.c" -Force

# Configure with CMake
Write-Host "Configuring with CMake..." -ForegroundColor Yellow
$configCmd = "cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp"
Write-Host "Running: $configCmd" -ForegroundColor Gray

# Use Invoke-Expression to bypass execution policy
Invoke-Expression $configCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    # Restore original files
    if (Test-Path "src/main.c.backup_v5") {
        Copy-Item "src/main.c.backup_v5" "src/main.c" -Force
        Remove-Item "src/main.c.backup_v5"
    }
    if (Test-Path "src/ble/ble_central.c.backup_v5") {
        Copy-Item "src/ble/ble_central.c.backup_v5" "src/ble/ble_central.c" -Force
        Remove-Item "src/ble/ble_central.c.backup_v5"
    }
    exit 1
}

Write-Host "CMake configuration successful!" -ForegroundColor Green

# Build with Ninja
Write-Host "Building with Ninja..." -ForegroundColor Yellow
$buildCmd = "ninja -C build"
Write-Host "Running: $buildCmd" -ForegroundColor Gray

Invoke-Expression $buildCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    # Restore original files
    if (Test-Path "src/main.c.backup_v5") {
        Copy-Item "src/main.c.backup_v5" "src/main.c" -Force
        Remove-Item "src/main.c.backup_v5"
    }
    if (Test-Path "src/ble/ble_central.c.backup_v5") {
        Copy-Item "src/ble/ble_central.c.backup_v5" "src/ble/ble_central.c" -Force
        Remove-Item "src/ble/ble_central.c.backup_v5"
    }
    exit 1
}

Write-Host ""
Write-Host "Build completed successfully!" -ForegroundColor Green

# Copy hex file to compiled_code with descriptive name
$hexSource = "build/zephyr/zephyr.hex"
$hexDest = "../../compiled_code/host_device_test_fixed_led_v5_$(Get-Date -Format 'yyyyMMdd')_rev018.hex"

if (Test-Path $hexSource) {
    Copy-Item $hexSource $hexDest -Force
    Write-Host "Firmware copied to: $hexDest" -ForegroundColor Cyan
    
    # Display file info
    $fileInfo = Get-Item $hexDest
    Write-Host "File size: $([math]::Round($fileInfo.Length / 1KB, 2)) KB" -ForegroundColor White
}

# Restore original files
Write-Host "Restoring original files..." -ForegroundColor Yellow
if (Test-Path "src/main.c.backup_v5") {
    Copy-Item "src/main.c.backup_v5" "src/main.c" -Force
    Remove-Item "src/main.c.backup_v5"
}
if (Test-Path "src/ble/ble_central.c.backup_v5") {
    Copy-Item "src/ble/ble_central.c.backup_v5" "src/ble/ble_central.c" -Force
    Remove-Item "src/ble/ble_central.c.backup_v5"
}

Write-Host ""
Write-Host "===== Test Firmware v5 Build Complete =====" -ForegroundColor Green
Write-Host "LED behavior fixed:" -ForegroundColor Yellow
Write-Host "- LED1: MotoApp connection only" -ForegroundColor White
Write-Host "- LED2: OFF (reserved for Mipe)" -ForegroundColor White
Write-Host "- LED3: Toggles during streaming" -ForegroundColor White
Write-Host ""
Write-Host "To flash: nrfjprog --program $hexDest --chiperase --verify -r" -ForegroundColor Cyan
