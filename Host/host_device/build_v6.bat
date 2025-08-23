@echo off
echo Building Host Device Test Firmware v6...

REM Set up nRF Connect SDK environment
set ZEPHYR_BASE=C:\ncs\v3.1.0\zephyr
set ZEPHYR_TOOLCHAIN_VARIANT=zephyr
set GNUARMEMB_TOOLCHAIN_PATH=C:\ncs\toolchains\b8b84efebd\opt
set PATH=C:\ncs\toolchains\b8b84efebd\opt\bin;C:\ncs\toolchains\b8b84efebd\opt\bin\Scripts;%PATH%

REM Copy v6 files
echo Copying v6 source files...
copy /Y src\main_test_fixed_rssi_v6.c src\main.c
copy /Y src\ble\ble_central_test.c src\ble\ble_central.c

REM Clean build directory
if exist build rmdir /s /q build

REM Configure with CMake
echo Configuring with CMake...
cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp

if %errorlevel% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

REM Build with Ninja
echo Building with Ninja...
ninja -C build

if %errorlevel% neq 0 (
    echo Build failed!
    exit /b 1
)

REM Copy output
echo Copying output file...
copy build\zephyr\zephyr.hex ..\..\compiled_code\host_device_test_alternating_rssi_v6_20250823_rev019.hex

echo.
echo ===== Build Complete =====
echo Output: compiled_code\host_device_test_alternating_rssi_v6_20250823_rev019.hex
echo.
echo Features:
echo - Alternates: Fixed -55 (LED3) / Real RSSI (LED2)
echo - LED1: MotoApp connection
