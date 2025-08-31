# SinglePing Project - Build Both Devices
# This script builds both Host and Mipe devices

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   SinglePing Project - Build Both Devices" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "Setting up Nordic Connect SDK environment..." -ForegroundColor Yellow
Write-Host ""

# Set environment variables for nRF Connect SDK v3.1.0
$env:ZEPHYR_BASE = "C:\ncs\v3.1.0\zephyr"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\ncs\toolchains\b8b84efebd\opt\zephyr-sdk"

# Add Nordic toolchain to PATH
$toolchainPath = "C:\ncs\toolchains\b8b84efebd\opt\bin;C:\ncs\toolchains\b8b84efebd\opt\bin\Scripts"
if ($env:PATH -notlike "*$toolchainPath*") {
    $env:PATH = "$toolchainPath;$env:PATH"
}

Write-Host "Environment configured:" -ForegroundColor Green
Write-Host "  ZEPHYR_BASE: $env:ZEPHYR_BASE"
Write-Host "  ZEPHYR_TOOLCHAIN_VARIANT: $env:ZEPHYR_TOOLCHAIN_VARIANT"
Write-Host "  ZEPHYR_SDK_INSTALL_DIR: $env:ZEPHYR_SDK_INSTALL_DIR"
Write-Host ""

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Building HOST DEVICE..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Build Host device
Set-Location "Host\host_device"
Write-Host "Working directory: $(Get-Location)" -ForegroundColor Cyan
Write-Host ""

try {
    Write-Host "Running Host device build..." -ForegroundColor Yellow
    & ".\build_west.ps1"
    if ($LASTEXITCODE -ne 0) {
        throw "Host device build failed with exit code $LASTEXITCODE"
    }
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "Host Device Build: SUCCESS" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
}
catch {
    Write-Host ""
    Write-Host "ERROR: Host device build failed!" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host ""
    exit 1
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Building MIPE DEVICE..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Build Mipe device
Set-Location "..\..\Mipe\mipe_device"
Write-Host "Working directory: $(Get-Location)" -ForegroundColor Cyan
Write-Host ""

try {
    Write-Host "Running Mipe device build..." -ForegroundColor Yellow
    & ".\build.ps1"
    if ($LASTEXITCODE -ne 0) {
        throw "Mipe device build failed with exit code $LASTEXITCODE"
    }
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "Mipe Device Build: SUCCESS" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
}
catch {
    Write-Host ""
    Write-Host "ERROR: Mipe device build failed!" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host ""
    exit 1
}

Write-Host "========================================" -ForegroundColor Green
Write-Host "ALL BUILDS COMPLETED SUCCESSFULLY!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

Write-Host "Build artifacts created:" -ForegroundColor Cyan
Write-Host ""
Write-Host "HOST DEVICE:" -ForegroundColor Yellow
Write-Host "  - build/host_device/zephyr/zephyr.hex"
Write-Host "  - build/host_device/zephyr/zephyr.elf"
Write-Host ""
Write-Host "MIPE DEVICE:" -ForegroundColor Yellow
Write-Host "  - build/zephyr/zephyr.hex"
Write-Host "  - build/zephyr/zephyr.elf"
Write-Host ""

Write-Host "To flash devices:" -ForegroundColor Cyan
Write-Host "  Host: nrfjprog --program Host\host_device\build\host_device\zephyr\zephyr.hex --chiperase --verify -r" -ForegroundColor White
Write-Host "  Mipe: nrfjprog --program Mipe\mipe_device\build\zephyr\zephyr.hex --chiperase --verify -r" -ForegroundColor White
Write-Host ""

Write-Host "Press any key to continue..." -ForegroundColor Gray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
