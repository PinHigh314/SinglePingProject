@echo off
echo ========================================
echo    SinglePing Project - Build Mipe Device
echo    USING PROVEN DIRECT CMAKE METHOD
echo ========================================
echo.

echo Setting up Nordic Connect SDK environment...
echo.

REM Set environment variables for nRF Connect SDK v3.1.0
set ZEPHYR_BASE=C:/ncs/v3.1.0/zephyr
set ZEPHYR_TOOLCHAIN_VARIANT=zephyr
set ZEPHYR_SDK_INSTALL_DIR=C:/ncs/toolchains/b8b84efebd/opt/zephyr-sdk

REM Add Nordic toolchain to PATH
set PATH=C:\ncs\toolchains\b8b84efebd\opt\bin;%PATH%

echo Environment configured:
echo   ZEPHYR_BASE: %ZEPHYR_BASE%
echo   ZEPHYR_TOOLCHAIN_VARIANT: %ZEPHYR_TOOLCHAIN_VARIANT%
echo   ZEPHYR_SDK_INSTALL_DIR: %ZEPHYR_SDK_INSTALL_DIR%
echo.

echo ========================================
echo Building MIPE DEVICE with DIRECT CMAKE...
echo ========================================
echo.

REM Navigate to project root first, then to Mipe device directory
cd /d "C:\Development\SinglePingProject"
echo Project root: %CD%
cd Mipe
echo Working directory: %CD%
echo.

REM Verify we're in the correct directory with CMakeLists.txt
if not exist CMakeLists.txt (
    echo ERROR: CMakeLists.txt not found in current directory!
    echo Expected: %CD%
    echo.
    echo Troubleshooting:
    echo   1. Ensure you're running from the project root
    echo   2. Check that Mipe directory exists
    echo   3. Verify CMakeLists.txt is present
    echo.
    pause
    exit /b 1
)

echo CMakeLists.txt found - proceeding with build...
echo.

REM Clean previous build if it exists
if exist build (
    echo Cleaning previous build directory...
    rmdir /s /q build
    echo Build directory cleaned.
    echo.
)

echo Starting DIRECT CMAKE build for Mipe device...
echo Board: nrf54l15dk/nrf54l15/cpuapp
echo Source directory: %CD%
echo.

REM Step 1: Configure build with Ninja generator (EXACT command that works)
echo Configuring build with CMake using Ninja generator...
echo Using ZEPHYR_BASE: %ZEPHYR_BASE%
cmake -G "Ninja" -B build -DBOARD=nrf54l15dk/nrf54l15/cpuapp -DZEPHYR_BASE="%ZEPHYR_BASE%"
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo.
    echo Troubleshooting tips:
    echo   1. Ensure Nordic Connect SDK v3.1.0 is installed
    echo   2. Check that ZEPHYR_BASE points to correct zephyr installation
    echo   3. Verify board target format is correct: nrf54l15dk/nrf54l15/cpuapp
    echo   4. Current working directory: %CD%
    echo   5. ZEPHYR_BASE used: %ZEPHYR_BASE%
    echo   6. Ensure Ninja build system is available
    echo.
    pause
    exit /b 1
)

echo.
echo CMake configuration successful!
echo.

REM Step 2: Build using Ninja (EXACT command that works)
echo Building with Ninja...
cmake --build build
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: CMake build failed!
    echo.
    echo Troubleshooting tips:
    echo   1. Check for compilation errors in the output above
    echo   2. Verify all source files are included in CMakeLists.txt
    echo   3. Check that prj.conf has correct configuration
    echo   4. Current working directory: %CD%
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Mipe Device Build: SUCCESS
echo ========================================
echo.

echo Build artifacts created:
if exist build\zephyr\zephyr.hex (
    echo   - build/zephyr/zephyr.hex
    for %%F in (build\zephyr\zephyr.hex) do echo     Size: %%~zF bytes
) else (
    echo   - build/zephyr/zephyr.hex (NOT FOUND)
)

if exist build\zephyr\zephyr.elf (
    echo   - build/zephyr/zephyr.elf
    for %%F in (build\zephyr\zephyr.elf) do echo     Size: %%~zF bytes
) else (
    echo   - build/zephyr/zephyr.elf (NOT FOUND)
)

if exist build\zephyr\zephyr.bin (
    echo   - build/zephyr/zephyr.bin
    for %%F in (build\zephyr\zephyr.bin) do echo     Size: %%~zF bytes
) else (
    echo   - build/zephyr/zephyr.bin (NOT FOUND)
)

echo.
echo Build completed successfully using DIRECT CMAKE method!
echo.
echo To flash the Mipe device:
echo   nrfjprog --program build\zephyr\zephyr.hex --chiperase --verify -r
echo.
echo Or use nRF Connect Programmer app
echo.
echo NOTE: This build used the PROVEN Direct CMake method
echo       with Ninja generator instead of problematic NMake.
echo.

pause
