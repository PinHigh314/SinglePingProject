# Build script for test fixed RSSI v6 with alternating transmission
# Using the same approach as v5 with cmake and ninja

Write-Host "Building Host Device Test Firmware v6 (Alternating RSSI)..." -ForegroundColor Cyan
Write-Host "Output: host_device_test_alternating_rssi_v6_20250823_rev019.hex" -ForegroundColor Yellow
Write-Host ""
Write-Host "Features in v6:" -ForegroundColor Green
Write-Host "- Alternates between fixed -55 dBm and real RSSI readings" -ForegroundColor White
Write-Host "- LED3 flashes for fixed reference values" -ForegroundColor White
Write-Host "- LED2 flashes for real RSSI values" -ForegroundColor White
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
    Copy-Item "src/main.c" "src/main.c.backup_v6" -Force
}

# Install v6 test version
Write-Host "Installing v6 test version..." -ForegroundColor Yellow
Copy-Item "src/main_test_fixed_rssi_v6.c" "src/main.c" -Force

# Backup and replace ble_central.c with test version
Write-Host "Installing test BLE central..." -ForegroundColor Yellow
if (Test-Path "src/ble/ble_central.c") {
    Copy-Item "src/ble/ble_central.c" "src/ble/ble_central.c.backup_v6" -Force
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
    if (Test-Path "src/main.c.backup_v6") {
        Copy-Item "src/main.c.backup_v6" "src/main.c" -Force
        Remove-Item "src/main.c.backup_v6"
    }
    if (Test-Path "src/ble/ble_central.c.backup_v6") {
        Copy-Item "src/ble/ble_central.c.backup_v6" "src/ble/ble_central.c" -Force
        Remove-Item "src/ble/ble_central.c.backup_v6"
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
    if (Test-Path "src/main.c.backup_v6") {
        Copy-Item "src/main.c.backup_v6" "src/main.c" -Force
        Remove-Item "src/main.c.backup_v6"
    }
    if (Test-Path "src/ble/ble_central.c.backup_v6") {
        Copy-Item "src/ble/ble_central.c.backup_v6" "src/ble/ble_central.c" -Force
        Remove-Item "src/ble/ble_central.c.backup_v6"
    }
    exit 1
}

Write-Host ""
Write-Host "Build completed successfully!" -ForegroundColor Green

# Copy hex file to compiled_code with descriptive name
$hexSource = "build/zephyr/zephyr.hex"
$hexDest = "../../compiled_code/host_device_test_alternating_rssi_v6_20250823_rev019.hex"

if (Test-Path $hexSource) {
    Copy-Item $hexSource $hexDest -Force
    Write-Host "Firmware copied to: $hexDest" -ForegroundColor Cyan
    
    # Display file info
    $fileInfo = Get-Item $hexDest
    Write-Host "File size: $([math]::Round($fileInfo.Length / 1KB, 2)) KB" -ForegroundColor White
}

# Restore original files
Write-Host "Restoring original files..." -ForegroundColor Yellow
if (Test-Path "src/main.c.backup_v6") {
    Copy-Item "src/main.c.backup_v6" "src/main.c" -Force
    Remove-Item "src/main.c.backup_v6"
}
if (Test-Path "src/ble/ble_central.c.backup_v6") {
    Copy-Item "src/ble/ble_central.c.backup_v6" "src/ble/ble_central.c" -Force
    Remove-Item "src/ble/ble_central.c.backup_v6"
}

Write-Host ""
Write-Host "===== Test Firmware v6 Build Complete =====" -ForegroundColor Green
Write-Host "Alternating RSSI transmission:" -ForegroundColor Yellow
Write-Host "- LED3: Flashes for fixed -55 dBm reference" -ForegroundColor White
Write-Host "- LED2: Flashes for real RSSI readings" -ForegroundColor White
Write-Host ""
Write-Host "To flash: nrfjprog --program $hexDest --chiperase --verify -r" -ForegroundColor Cyan
