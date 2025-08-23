# Universal build script for test firmware versions
# Usage: .\build_test_universal.ps1 -Version <version_number> -Description <description>

param(
    [Parameter(Mandatory=$true)]
    [string]$Version,
    
    [Parameter(Mandatory=$true)]
    [string]$Description,
    
    [string]$OutputPrefix = "host_device_test",
    
    [string]$RevNumber = "999"  # Default rev number if not specified
)

Write-Host "Building Host Device Test Firmware v$Version" -ForegroundColor Cyan
Write-Host "Description: $Description" -ForegroundColor Yellow
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

# Check if source file exists
$sourceFile = "src\main_test_fixed_rssi_v$Version.c"
if (-not (Test-Path $sourceFile)) {
    Write-Host "Error: Source file $sourceFile not found!" -ForegroundColor Red
    exit 1
}

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

# Install test version files
Write-Host "Installing test version v$Version..." -ForegroundColor Yellow
Copy-Item $sourceFile "src/main.c" -Force
Copy-Item "src/ble/ble_central_test.c" "src/ble/ble_central.c" -Force

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

# Generate output filename
$date = Get-Date -Format 'yyyyMMdd'
$outputName = $Description.ToLower() -replace ' ', '_' -replace '[^a-z0-9_]', ''
$hexDest = "../../compiled_code/${OutputPrefix}_${outputName}_v${Version}_${date}_rev${RevNumber}.hex"

# Copy hex file
$hexSource = "build/zephyr/zephyr.hex"
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

Write-Host ""
Write-Host "===== Build Complete =====" -ForegroundColor Green
Write-Host "Version: v$Version" -ForegroundColor Yellow
Write-Host "Description: $Description" -ForegroundColor Yellow
Write-Host "Output: $hexDest" -ForegroundColor Cyan
Write-Host ""
Write-Host "To flash: nrfjprog --program $hexDest --chiperase --verify -r" -ForegroundColor Cyan
