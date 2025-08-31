# SinglePing Zephyr Environment Setup Script
# Run this script to set up Zephyr environment variables

Write-Host "========================================" -ForegroundColor Green
Write-Host "SinglePing Zephyr Environment Setup" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

# Set Zephyr SDK location (external to project)
$env:ZEPHYR_BASE = "C:\zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\ncs\toolchains\b8b84efebd\opt\zephyr-sdk"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"

# Add toolchain to PATH
$toolchainPath = "C:\ncs\toolchains\b8b84efebd\opt\bin;C:\ncs\toolchains\b8b84efebd\opt\bin\Scripts"
$env:PATH = "$toolchainPath;$env:PATH"

Write-Host "Environment variables set:" -ForegroundColor Yellow
Write-Host "  ZEPHYR_BASE: $env:ZEPHYR_BASE" -ForegroundColor Cyan
Write-Host "  ZEPHYR_SDK_INSTALL_DIR: $env:ZEPHYR_SDK_INSTALL_DIR" -ForegroundColor Cyan
Write-Host "  ZEPHYR_TOOLCHAIN_VARIANT: $env:ZEPHYR_TOOLCHAIN_VARIANT" -ForegroundColor Cyan
Write-Host ""
Write-Host "Toolchain added to PATH" -ForegroundColor Green
Write-Host ""

# Check if Zephyr base exists
if (Test-Path $env:ZEPHYR_BASE) {
    Write-Host "✓ Zephyr SDK found at: $env:ZEPHYR_BASE" -ForegroundColor Green
} else {
    Write-Host "⚠ Warning: Zephyr SDK not found at: $env:ZEPHYR_BASE" -ForegroundColor Yellow
    Write-Host "  Please ensure you have moved the SDK from sdk-zephyr/ to C:\zephyr" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "To make these permanent, add to your system environment variables:" -ForegroundColor Yellow
Write-Host "  ZEPHYR_BASE = C:\zephyr" -ForegroundColor Cyan
Write-Host "  ZEPHYR_SDK_INSTALL_DIR = C:\ncs\toolchains\b8b84efebd\opt\zephyr-sdk" -ForegroundColor Cyan
Write-Host "  ZEPHYR_TOOLCHAIN_VARIANT = zephyr" -ForegroundColor Cyan
Write-Host ""
Write-Host "Ready for Zephyr development!" -ForegroundColor Green
Write-Host ""
