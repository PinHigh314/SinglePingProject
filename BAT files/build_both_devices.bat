@echo off
echo ========================================
echo    SinglePing Project - Build Both Devices
echo ========================================
echo.

echo Setting up Nordic Connect SDK environment...
echo.

REM Set environment variables for nRF Connect SDK v3.1.0
set ZEPHYR_BASE=C:\ncs\v3.1.0\zephyr
set ZEPHYR_TOOLCHAIN_VARIANT=zephyr
set ZEPHYR_SDK_INSTALL_DIR=C:\ncs\toolchains\b8b84efebd\opt\zephyr-sdk

REM Add Nordic toolchain to PATH
set PATH=C:\ncs\toolchains\b8b84efebd\opt\bin;%PATH%

echo Environment configured:
echo   ZEPHYR_BASE: %ZEPHYR_BASE%
echo   ZEPHYR_TOOLCHAIN_VARIANT: %ZEPHYR_TOOLCHAIN_VARIANT%
echo   ZEPHYR_SDK_INSTALL_DIR: %ZEPHYR_SDK_INSTALL_DIR%
echo.

echo ========================================
echo Building HOST DEVICE...
echo ========================================
echo.

REM Navigate to project root first, then to Host device directory
cd /d "C:\Development\SinglePingProject"
echo Project root: %CD%
cd Host\host_device
echo Working directory: %CD%
echo.

REM Verify we're in the correct directory with CMakeLists.txt
if not exist CMakeLists.txt (
    echo ERROR: CMakeLists.txt not found in current directory!
    echo Expected: %CD%
    echo.
    echo Troubleshooting:
    echo   1. Ensure you're running from the project root
    echo   2. Check that Host\host_device directory exists
    echo   3. Verify CMakeLists.txt is present
    echo.
    pause
    exit /b 1
)

echo CMakeLists.txt found - proceeding with Host build...
echo.

REM Clean previous build if it exists
if exist build (
    echo Cleaning previous build directory...
    rmdir /s /q build
    echo Build directory cleaned.
    echo.
)

echo Starting west build for Host device...
echo Board: nrf54l15dk/nrf54l15/cpuapp
echo Source directory: %CD%
echo.

REM Run Host device build with west
west build --board nrf54l15dk/nrf54l15/cpuapp
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Host device build failed!
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Host Device Build: SUCCESS
echo ========================================
echo.

REM Return to project root
cd /d "C:\Development\SinglePingProject"
echo Returned to project root: %CD%

echo ========================================
echo Building MIPE DEVICE...
echo ========================================
echo.

cd Mipe\mipe_device
echo Working directory: %CD%
echo.

REM Verify we're in the correct directory with CMakeLists.txt
if not exist CMakeLists.txt (
    echo ERROR: CMakeLists.txt not found in current directory!
    echo Expected: %CD%
    echo.
    echo Troubleshooting:
    echo   1. Ensure you're running from the project root
    echo   2. Check that Mipe\mipe_device directory exists
    echo   3. Verify CMakeLists.txt is present
    echo.
    pause
    exit /b 1
)

echo CMakeLists.txt found - proceeding with Mipe build...
echo.

REM Clean previous build if it exists
if exist build (
    echo Cleaning previous build directory...
    rmdir /s /q build
    echo Build directory cleaned.
    echo.
)

echo Starting west build for Mipe device...
echo Board: nrf54l15dk/nrf54l15/cpuapp
echo Source directory: %CD%
echo.

REM Run Mipe device build with west
west build --board nrf54l15dk/nrf54l15/cpuapp
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Mipe device build failed!
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Mipe Device Build: SUCCESS
echo ========================================
echo.

REM Return to project root
cd /d "C:\Development\SinglePingProject"
echo Returned to project root: %CD%

echo.
echo ========================================
echo ALL BUILDS COMPLETED SUCCESSFULLY!
echo ========================================
echo.

echo Build artifacts created:
echo.
echo HOST DEVICE:
if exist Host\host_device\build\zephyr\zephyr.hex (
    echo   - Host\host_device\build\zephyr\zephyr.hex
    for %%F in (Host\host_device\build\zephyr\zephyr.hex) do echo     Size: %%~zF bytes
) else (
    echo   - Host\host_device\build\zephyr\zephyr.hex (NOT FOUND)
)

echo.
echo MIPE DEVICE:
if exist Mipe\mipe_device\build\zephyr\zephyr.hex (
    echo   - Mipe\mipe_device\build\zephyr\zephyr.hex
    for %%F in (Mipe\mipe_device\build\zephyr\zephyr.hex) do echo     Size: %%~zF bytes
) else (
    echo   - Mipe\mipe_device\build\zephyr\zephyr.hex (NOT FOUND)
)

echo.
echo ========================================
echo Build Summary
echo ========================================
echo.
echo To flash the Host device:
echo   nrfjprog --program Host\host_device\build\zephyr\zephyr.hex --chiperase --verify -r
echo.
echo To flash the Mipe device:
echo   nrfjprog --program Mipe\mipe_device\build\zephyr\zephyr.hex --chiperase --verify -r
echo.
echo Or use nRF Connect Programmer app for both devices
echo.

pause
