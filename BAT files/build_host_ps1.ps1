# SinglePing Project - Build Host Device (PowerShell)
# This script builds the Host device directly without sysbuild

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   SinglePing Project - Build Host Device" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "Setting up Nordic Connect SDK environment..." -ForegroundColor Yellow
Write-Host ""

# Set environment variables for nRF Connect SDK v3.1.0
$env:ZEPHYR_BASE = "C:\ncs\v3.1.0\zephyr"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\ncs\toolchains\b8b84efebd\opt\zephyr-sdk"

# Add Nordic toolchain to PATH
$env:PATH = "C:\ncs\toolchains\b8b84efebd\opt\bin;" + $env:PATH

Write-Host "Environment configured:" -ForegroundColor Green
Write-Host "  ZEPHYR_BASE: $env:ZEPHYR_BASE" -ForegroundColor White
Write-Host "  ZEPHYR_TOOLCHAIN_VARIANT: $env:ZEPHYR_TOOLCHAIN_VARIANT" -ForegroundColor White
Write-Host "  ZEPHYR_SDK_INSTALL_DIR: $env:ZEPHYR_SDK_INSTALL_DIR" -ForegroundColor White
Write-Host ""

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Building HOST DEVICE (Direct Method)..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Navigate to Host device directory
Set-Location "C:\Development\SinglePingProject\Host\host_device"
Write-Host "Working directory: $(Get-Location)" -ForegroundColor Green
Write-Host ""

# Verify we're in the correct directory with CMakeLists.txt
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Host "ERROR: CMakeLists.txt not found in current directory!" -ForegroundColor Red
    Write-Host "Expected: $(Get-Location)" -ForegroundColor Red
    Read-Host "Press Enter to continue"
    exit 1
}

Write-Host "CMakeLists.txt found - proceeding with build..." -ForegroundColor Green
Write-Host ""

# Clean previous build if it exists
if (Test-Path "build") {
    Write-Host "Cleaning previous build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build"
    Write-Host "Build directory cleaned." -ForegroundColor Green
    Write-Host ""
}

Write-Host "Starting direct build for Host device..." -ForegroundColor Yellow
Write-Host "Board: nrf54l15dk/nrf54l15/cpuapp" -ForegroundColor White
Write-Host "Source directory: $(Get-Location)" -ForegroundColor White
Write-Host ""

# Try building with explicit source directory to avoid sysbuild
Write-Host "Attempting west build with explicit source directory..." -ForegroundColor Yellow
$westResult = & west build --board nrf54l15dk/nrf54l15/cpuapp --source-dir . --build-dir build 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: Direct west build failed!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Trying alternative build method with CMake directly..." -ForegroundColor Yellow
    Write-Host ""
    
    # Try building with CMake directly (bypass west)
    if (Test-Path "build") {
        Remove-Item -Recurse -Force "build"
    }
    New-Item -ItemType Directory -Name "build" | Out-Null
    Set-Location "build"
    
    Write-Host "Running CMake configuration..." -ForegroundColor Yellow
    $cmakeResult = & cmake -GNinja -DBOARD=nrf54l15dk/nrf54l15/cpuapp .. 2>&1
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host ""
        Write-Host "ERROR: CMake configuration failed!" -ForegroundColor Red
        Write-Host "CMake output: $cmakeResult" -ForegroundColor Red
        Read-Host "Press Enter to continue"
        exit 1
    }
    
    Write-Host "Running Ninja build..." -ForegroundColor Yellow
    $ninjaResult = & ninja 2>&1
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host ""
        Write-Host "ERROR: Ninja build failed!" -ForegroundColor Red
        Write-Host "Ninja output: $ninjaResult" -ForegroundColor Red
        Read-Host "Press Enter to continue"
        exit 1
    }
    
    Write-Host ""
    Write-Host "Build completed successfully with CMake+Ninja!" -ForegroundColor Green
    Write-Host ""
    
    # Return to source directory
    Set-Location ".."
} else {
    Write-Host ""
    Write-Host "Build completed successfully with west!" -ForegroundColor Green
    Write-Host ""
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Host Device Build: SUCCESS" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "Build artifacts created:" -ForegroundColor Green
if (Test-Path "build\zephyr\zephyr.hex") {
    $hexSize = (Get-Item "build\zephyr\zephyr.hex").Length
    Write-Host "  - build/zephyr/zephyr.hex" -ForegroundColor White
    Write-Host "    Size: $hexSize bytes" -ForegroundColor Gray
} else {
    Write-Host "  - build/zephyr/zephyr.hex (NOT FOUND)" -ForegroundColor Red
}

if (Test-Path "build\zephyr\zephyr.elf") {
    $elfSize = (Get-Item "build\zephyr\zephyr.elf").Length
    Write-Host "  - build/zephyr/zephyr.elf" -ForegroundColor White
    Write-Host "    Size: $elfSize bytes" -ForegroundColor Gray
} else {
    Write-Host "  - build/zephyr/zephyr.elf (NOT FOUND)" -ForegroundColor Red
}

Write-Host ""
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host ""
Write-Host "To flash the Host device:" -ForegroundColor Cyan
Write-Host "  nrfjprog --program build\zephyr\zephyr.hex --chiperase --verify -r" -ForegroundColor White
Write-Host ""
Write-Host "Or use nRF Connect Programmer app" -ForegroundColor White
Write-Host ""

Read-Host "Press Enter to continue"

