@echo off
setlocal

echo ========================================
echo SinglePing Host Device Build Script
echo Using stable cmake/ninja build process
echo ========================================
echo.
echo Usage: build-host-device.bat [options] [description]
echo   Options:
echo     --clean or -c : Clean build (removes build directory first)
echo   Description:
echo     Optional description for the output hex file
echo     Example: build-host-device.bat v9_restored
echo     Example: build-host-device.bat --clean test_dual_ble
echo.

:: Set environment variables for nRF Connect SDK v3.1.0
echo Setting up environment variables...
set ZEPHYR_BASE=C:\ncs\v3.1.0\zephyr
set ZEPHYR_SDK_INSTALL_DIR=C:\ncs\toolchains\b8b84efebd\opt\zephyr-sdk
set ZEPHYR_TOOLCHAIN_VARIANT=zephyr

:: Add toolchain to PATH
set PATH=C:\ncs\toolchains\b8b84efebd\opt\bin;C:\ncs\toolchains\b8b84efebd\opt\bin\Scripts;%PATH%

echo Environment configured:
echo   ZEPHYR_BASE: %ZEPHYR_BASE%
echo   ZEPHYR_SDK_INSTALL_DIR: %ZEPHYR_SDK_INSTALL_DIR%
echo   ZEPHYR_TOOLCHAIN_VARIANT: %ZEPHYR_TOOLCHAIN_VARIANT%
echo.

:: Check for clean build flag
set clean_build=0
if "%1"=="--clean" set clean_build=1
if "%1"=="-c" set clean_build=1

:: Navigate to host_device directory
cd /d "%~dp0Host\host_device"
echo Working directory: %CD%
echo.

:: Clean build directory if requested
if %clean_build%==1 (
    echo Clean build requested...
    if exist build (
        echo Removing existing build directory...
        rmdir /s /q build
    )
)

:: Check if build directory needs to be created
if not exist build\CMakeCache.txt (
    if exist build (
        echo Build directory exists but no CMakeCache.txt found, cleaning...
        rmdir /s /q build
    )
)

:: Configure with CMake
echo Configuring project with CMake...
echo Board: nrf54l15dk/nrf54l15/cpuapp
echo.

echo Running: cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp
cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp

if %errorlevel% neq 0 (
    echo.
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo CMake configuration successful!
echo.

:: Build with Ninja
echo Building project with Ninja...
echo Running: ninja -C build
ninja -C build

if %errorlevel% neq 0 (
    echo.
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build Completed Successfully!
echo ========================================
echo.

:: Display build artifacts
if exist build\zephyr\zephyr.hex (
    echo Build artifacts:
    echo   HEX file: %CD%\build\zephyr\zephyr.hex
    for %%F in (build\zephyr\zephyr.hex) do echo   Size: %%~zF bytes
)

if exist build\zephyr\zephyr.elf (
    echo   ELF file: %CD%\build\zephyr\zephyr.elf
    for %%F in (build\zephyr\zephyr.elf) do echo   Size: %%~zF bytes
)

echo.
echo To flash the device, use:
echo   nrfjprog --program build\zephyr\zephyr.hex --chiperase --verify -r
echo Or use the nRF Connect Programmer app
echo.

:: Get description from command line argument or use default
set description=%2
if "%description%"=="" set description=build

:: Get git revision number - try multiple methods
set rev_count=
for /f "tokens=*" %%i in ('git rev-list --count HEAD 2^>nul') do set rev_count=%%i

:: If git rev-list failed, try alternative method
if "%rev_count%"=="" (
    echo Git revision count failed, trying alternative method...
    for /f "tokens=*" %%i in ('git log --oneline 2^>nul ^| find /c /v "" ') do set rev_count=%%i
)

:: If still no revision, check if we're in a git repo
if "%rev_count%"=="" (
    git status >nul 2>&1
    if %errorlevel% neq 0 (
        echo Warning: Not in a git repository, using timestamp-based revision
        :: Use seconds since midnight as a unique number
        for /f "tokens=1-3 delims=:." %%a in ('echo %time%') do (
            set /a rev_count=%%a*3600+%%b*60+%%c
        )
    ) else (
        echo Warning: Git command failed, defaulting to 001
        set rev_count=001
    )
)

:: Format revision number with leading zeros
set "rev_num=00%rev_count%"
set "rev_num=%rev_num:~-3%"

echo Using revision number: %rev_num% (count: %rev_count%)

:: Get current date in YYYYMMDD format
for /f "tokens=2-4 delims=/ " %%a in ('date /t') do (
    set month=%%a
    set day=%%b
    set year=%%c
)
set datestamp=%year%%month%%day%

:: Create output filename
set output_name=host_device_%description%_%datestamp%_rev%rev_num%.hex

:: Copy hex file to compiled_code directory
echo Copying hex file to compiled_code directory...
if not exist "%~dp0compiled_code" mkdir "%~dp0compiled_code"
copy build\zephyr\zephyr.hex "%~dp0compiled_code\%output_name%" >nul

if %errorlevel% equ 0 (
    echo Hex file copied to: compiled_code\%output_name%
    
    :: Update version log
    echo %date% %time% - Built: %output_name% >> "%~dp0compiled_code\version_log.md"
) else (
    echo Warning: Failed to copy hex file to compiled_code directory
)

echo.
pause
