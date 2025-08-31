# ========================================
#    SinglePing Project - Build Mipe Device
#    USING PROVEN DIRECT CMAKE METHOD
# ========================================

Write-Host "Setting up Nordic Connect SDK environment..." -ForegroundColor Green
Write-Host ""

# Set environment variables for nRF Connect SDK v3.1.0
$env:ZEPHYR_BASE = "C:/ncs/v3.1.0/zephyr"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:/ncs/toolchains/b8b84efebd/opt/zephyr-sdk"

# Add Nordic toolchain to PATH
$env:PATH = "C:\ncs\toolchains\b8b84efebd\opt\bin;$env:PATH"

Write-Host "Environment configured:" -ForegroundColor Yellow
Write-Host "  ZEPHYR_BASE: $env:ZEPHYR_BASE" -ForegroundColor Cyan
Write-Host "  ZEPHYR_TOOLCHAIN_VARIANT: $env:ZEPHYR_TOOLCHAIN_VARIANT" -ForegroundColor Cyan
Write-Host "  ZEPHYR_SDK_INSTALL_DIR: $env:ZEPHYR_SDK_INSTALL_DIR" -ForegroundColor Cyan
Write-Host ""

Write-Host "========================================" -ForegroundColor Green
Write-Host "Building MIPE DEVICE with DIRECT CMAKE..." -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

# Navigate to project root first, then to Mipe device directory
Set-Location "C:\Development\SinglePingProject"
Write-Host "Project root: $(Get-Location)" -ForegroundColor Yellow
Set-Location "Mipe"
Write-Host "Working directory: $(Get-Location)" -ForegroundColor Yellow
Write-Host ""

# Verify we're in the correct directory with CMakeLists.txt
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Host "ERROR: CMakeLists.txt not found in current directory!" -ForegroundColor Red
    Write-Host "Expected: $(Get-Location)" -ForegroundColor Red
    Write-Host ""
    Write-Host "Troubleshooting:" -ForegroundColor Yellow
    Write-Host "  1. Ensure you're running from the project root" -ForegroundColor White
    Write-Host "  2. Check that Mipe directory exists" -ForegroundColor White
    Write-Host "  3. Verify CMakeLists.txt is present" -ForegroundColor White
    Write-Host ""
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

Write-Host "Starting DIRECT CMAKE build for Mipe device..." -ForegroundColor Green
Write-Host "Board: nrf54l15dk/nrf54l15/cpuapp" -ForegroundColor Cyan
Write-Host "Source directory: $(Get-Location)" -ForegroundColor Cyan
Write-Host ""

# Step 1: Configure build with Ninja generator (EXACT command that works)
Write-Host "Configuring build with CMake using Ninja generator..." -ForegroundColor Yellow
Write-Host "Using ZEPHYR_BASE: $env:ZEPHYR_BASE" -ForegroundColor Cyan
$configResult = cmake -G "Ninja" -B build -DBOARD=nrf54l15dk/nrf54l15/cpuapp -DZEPHYR_BASE="$env:ZEPHYR_BASE"
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: CMake configuration failed!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Troubleshooting tips:" -ForegroundColor Yellow
    Write-Host "  1. Ensure Nordic Connect SDK v3.1.0 is installed" -ForegroundColor White
    Write-Host "  2. Check that ZEPHYR_BASE points to correct zephyr installation" -ForegroundColor White
    Write-Host "  3. Verify board target format is correct: nrf54l15dk/nrf54l15/cpuapp" -ForegroundColor White
    Write-Host "  4. Current working directory: $(Get-Location)" -ForegroundColor White
    Write-Host "  5. ZEPHYR_BASE used: $env:ZEPHYR_BASE" -ForegroundColor White
    Write-Host "  6. Ensure Ninja build system is available" -ForegroundColor White
    Write-Host ""
    Read-Host "Press Enter to continue"
    exit 1
}

Write-Host ""
Write-Host "CMake configuration successful!" -ForegroundColor Green
Write-Host ""

# Step 2: Build using Ninja (EXACT command that works)
Write-Host "Building with Ninja..." -ForegroundColor Yellow
$buildResult = cmake --build build
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: CMake build failed!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Troubleshooting tips:" -ForegroundColor Yellow
    Write-Host "  1. Check for compilation errors in the output above" -ForegroundColor White
    Write-Host "  2. Verify all source files are included in CMakeLists.txt" -ForegroundColor White
    Write-Host "  3. Check that prj.conf has correct configuration" -ForegroundColor White
    Write-Host "  4. Current working directory: $(Get-Location)" -ForegroundColor White
    Write-Host ""
    Read-Host "Press Enter to continue"
    exit 1
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "Mipe Device Build: SUCCESS" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

Write-Host "Build artifacts created:" -ForegroundColor Yellow
if (Test-Path "build\zephyr\zephyr.hex") {
    $hexSize = (Get-Item "build\zephyr\zephyr.hex").Length
    Write-Host "  - build/zephyr/zephyr.hex" -ForegroundColor Green
    Write-Host "    Size: $hexSize bytes" -ForegroundColor Cyan
} else {
    Write-Host "  - build/zephyr/zephyr.hex (NOT FOUND)" -ForegroundColor Red
}

if (Test-Path "build\zephyr\zephyr.elf") {
    $elfSize = (Get-Item "build\zephyr\zephyr.elf").Length
    Write-Host "  - build/zephyr/zephyr.elf" -ForegroundColor Green
    Write-Host "    Size: $elfSize bytes" -ForegroundColor Cyan
} else {
    Write-Host "  - build/zephyr/zephyr.elf (NOT FOUND)" -ForegroundColor Red
}

if (Test-Path "build\zephyr\zephyr.bin") {
    $binSize = (Get-Item "build\zephyr\zephyr.bin").Length
    Write-Host "  - build/zephyr/zephyr.bin" -ForegroundColor Green
    Write-Host "    Size: $binSize bytes" -ForegroundColor Cyan
} else {
    Write-Host "  - build/zephyr/zephyr.bin (NOT FOUND)" -ForegroundColor Red
}

Write-Host ""
Write-Host "Build completed successfully using DIRECT CMAKE method!" -ForegroundColor Green
Write-Host ""
Write-Host "To flash the Mipe device:" -ForegroundColor Yellow
Write-Host "  nrfjprog --program build\zephyr\zephyr.hex --chiperase --verify -r" -ForegroundColor White
Write-Host ""
Write-Host "Or use nRF Connect Programmer app" -ForegroundColor White
Write-Host ""
Write-Host "NOTE: This build used the PROVEN Direct CMake method" -ForegroundColor Cyan
Write-Host "      instead of the problematic west build command." -ForegroundColor Cyan
Write-Host ""

Read-Host "Press Enter to continue"
