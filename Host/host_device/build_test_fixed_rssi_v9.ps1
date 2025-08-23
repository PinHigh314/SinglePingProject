# Build script for Host Device Test Fixed RSSI v9
# LED1 timing fix version

$ErrorActionPreference = "Stop"

Write-Host "=== Building Host Device Test Fixed RSSI v9 ===" -ForegroundColor Cyan
Write-Host "LED1 timing fix for proper solid ON state" -ForegroundColor Yellow

# Configuration
$BOARD = "nrf54l15dk/nrf54l15/cpuapp"
$BUILD_DIR = "build_test_fixed_rssi_v9"
$SOURCE_DIR = "."
$MAIN_FILE = "src/main_test_fixed_rssi_v9.c"
$OUTPUT_NAME = "host_device_test_led1_fix_v9"

# Get current date/time for build tracking
$BuildDate = Get-Date -Format "yyyyMMdd"
$BuildTime = Get-Date -Format "HHmmss"

# Clean previous build
if (Test-Path $BUILD_DIR) {
    Write-Host "Cleaning previous build directory..." -ForegroundColor Yellow
    Remove-Item -Path $BUILD_DIR -Recurse -Force
}

# Create CMakeLists.txt for v9
$cmakeContent = @'
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(host_device_test_v9)

# Main application source - v9 with LED1 timing fix
target_sources(app PRIVATE
    src/main_test_fixed_rssi_v9.c
    src/ble/ble_peripheral_v8.c
    src/ble/ble_central_real.c
)

# Include directories
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
'@

Write-Host "Creating CMakeLists.txt for v9..." -ForegroundColor Green
$cmakeContent | Out-File -FilePath "CMakeLists.txt" -Encoding UTF8

# Build the project using Invoke-Expression
Write-Host "`nBuilding firmware with west..." -ForegroundColor Green
Write-Host "Board: $BOARD" -ForegroundColor Cyan
Write-Host "Build directory: $BUILD_DIR" -ForegroundColor Cyan

# Activate nRF Connect SDK environment and build
$activateAndBuild = @"
C:\ncs\toolchains\ce3b5ff664\nrfutil.exe toolchain-manager launch --terminal -- west build -b $BOARD -d $BUILD_DIR $SOURCE_DIR -- -DCONFIG_COMPILER_WARNINGS_AS_ERRORS=n
"@

Write-Host "Executing build with nRF Connect SDK..." -ForegroundColor Gray

try {
    $result = Invoke-Expression $activateAndBuild
    Write-Host $result
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`nBuild completed successfully!" -ForegroundColor Green
        
        # Copy the hex file to compiled_code directory with descriptive name
        $hexSource = Join-Path $BUILD_DIR "zephyr/zephyr.hex"
        $hexDest = "../../compiled_code/${OUTPUT_NAME}_${BuildDate}_rev022.hex"
        
        if (Test-Path $hexSource) {
            Copy-Item -Path $hexSource -Destination $hexDest -Force
            Write-Host "`nHex file copied to: $hexDest" -ForegroundColor Green
            
            # Get file size
            $fileSize = (Get-Item $hexDest).Length / 1KB
            Write-Host "File size: $([math]::Round($fileSize, 2)) KB" -ForegroundColor Cyan
        }
        
        Write-Host "`n=== Build Summary ===" -ForegroundColor Cyan
        Write-Host "Version: v9 (LED1 timing fix)" -ForegroundColor Yellow
        Write-Host "Main changes:" -ForegroundColor Yellow
        Write-Host "  - Added update_led1_state() helper function" -ForegroundColor White
        Write-Host "  - Timer always stopped before LED1 state changes" -ForegroundColor White
        Write-Host "  - Added 10ms delay after timer stop" -ForegroundColor White
        Write-Host "  - Centralized LED1 control logic" -ForegroundColor White
        Write-Host "  - Enhanced logging for LED1 state changes" -ForegroundColor White
        Write-Host "`nLED1 behavior:" -ForegroundColor Yellow
        Write-Host "  - Solid ON: MotoApp connected only" -ForegroundColor Green
        Write-Host "  - Rapid flash (100ms): Both MotoApp and Mipe connected" -ForegroundColor Green
        Write-Host "  - OFF: No MotoApp connection" -ForegroundColor Green
        
    } else {
        Write-Host "`nBuild failed with exit code: $LASTEXITCODE" -ForegroundColor Red
        exit $LASTEXITCODE
    }
} catch {
    Write-Host "`nBuild error: $_" -ForegroundColor Red
    exit 1
}
