@echo off
setlocal

echo ========================================
echo SinglePing Project - Build Both Devices
echo Using stable cmake/ninja build process
echo ========================================
echo.
echo Usage: build_both_devices.bat [options] [description]
echo   Options:
echo     --clean or -c : Clean build (removes build directory first)
echo   Description:
echo     Optional description for the output hex files
echo     Example: build_both_devices.bat v9_restored
echo     Example: build_both_devices.bat --clean test_dual_ble
echo.

:: Set environment variables for external Zephyr SDK
echo Setting up environment variables...
set ZEPHYR_BASE=C:\zephyr
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

:: Get description from command line argument or use default
set description=%2
if "%description%"=="" set description=build

:: Get current date and time for versioning with proper formatting
for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value 2^>nul') do set "dt=%%a"
if "%dt%"=="" (
    REM Fallback to PowerShell if wmic not available
    for /f "tokens=1-5 delims=: " %%a in ('powershell -Command "Get-Date -Format 'yy MM dd HH mm'"') do (
        set "YY=%%a"
        set "MM=%%b"
        set "DD=%%c"
        set "HH=%%d"
        set "Min=%%e"
    )
) else (
    set "YY=%dt:~2,2%"
    set "MM=%dt:~4,2%"
    set "DD=%dt:~6,2%"
    set "HH=%dt:~8,2%"
    set "Min=%dt:~10,2%"
)

REM Ensure 2-digit minutes
if "%Min%"=="" set "Min=00"
if "%Min:~1,1%"=="" set "Min=0%Min%"

REM Create timestamp (YYMMDD_HHMM)
set "timestamp=%YY%%MM%%DD%_%HH%%Min%"

echo ========================================
echo Building HOST DEVICE...
echo ========================================
echo.

:: Navigate to Host device directory
cd /d "%~dp0..\Host\host_device"
echo Working directory: %CD%
echo.

:: Check if CMakeLists.txt exists
if not exist CMakeLists.txt (
    echo ERROR: CMakeLists.txt not found in %CD%
    echo Please ensure you're in the correct directory
    pause
    exit /b 1
)

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
echo Configuring Host device with CMake...
echo Board: nrf54l15dk/nrf54l15/cpuapp
echo.

echo Running: cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp
cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp

if %errorlevel% neq 0 (
    echo.
    echo Host device CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Host device CMake configuration successful!
echo.

:: Build with Ninja
echo Building Host device with Ninja...
echo Running: ninja -C build
ninja -C build

if %errorlevel% neq 0 (
    echo.
    echo Host device build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Host Device Build: SUCCESS
echo ========================================
echo.

:: Return to project root
cd /d "%~dp0.."
echo Returned to project root: %CD%

echo ========================================
echo Building MIPE DEVICE...
echo ========================================
echo.

cd Mipe
echo Working directory: %CD%
echo.

:: Check if CMakeLists.txt exists
if not exist CMakeLists.txt (
    echo ERROR: CMakeLists.txt not found in %CD%
    echo Please ensure you're in the correct directory
    pause
    exit /b 1
)

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
echo Configuring Mipe device with CMake...
echo Board: nrf54l15dk/nrf54l15/cpuapp
echo.

echo Running: cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp
cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp

if %errorlevel% neq 0 (
    echo.
    echo Mipe device CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Mipe device CMake configuration successful!
echo.

:: Build with Ninja
echo Building Mipe device with Ninja...
echo Running: ninja -C build
ninja -C build

if %errorlevel% neq 0 (
    echo.
    echo Mipe device build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Mipe Device Build: SUCCESS
echo ========================================
echo.

:: Return to project root
cd /d "%~dp0.."
echo Returned to project root: %CD%

echo.
echo ========================================
echo ALL BUILDS COMPLETED SUCCESSFULLY!
echo ========================================
echo.

:: Create output filenames
set host_output_name=Host_%timestamp%.hex
set mipe_output_name=Mipe_%timestamp%.hex

:: Copy hex files to compiled_code directory
echo Copying hex files to compiled_code directory...
if not exist "compiled_code" mkdir "compiled_code"

:: Copy Host device hex file
if exist Host\host_device\build\zephyr\zephyr.hex (
    copy Host\host_device\build\zephyr\zephyr.hex "compiled_code\%host_output_name%" >nul
    if %errorlevel% equ 0 (
        echo Host hex file copied to: compiled_code\%host_output_name%
    ) else (
        echo Warning: Failed to copy Host hex file
    )
) else (
    echo Warning: Host hex file not found
)

:: Copy Mipe device hex file
if exist Mipe\build\zephyr\zephyr.hex (
    copy Mipe\build\zephyr\zephyr.hex "compiled_code\%mipe_output_name%" >nul
    if %errorlevel% equ 0 (
        echo Mipe hex file copied to: compiled_code\%mipe_output_name%
    ) else (
        echo Warning: Failed to copy Mipe hex file
    )
) else (
    echo Warning: Mipe hex file not found
)

:: Update version log
echo %date% %time% - Built both devices: %host_output_name%, %mipe_output_name% >> "compiled_code\version_log.md"

echo.
echo ========================================
echo Build Summary
echo ========================================
echo.

echo Build artifacts created:
echo.
echo HOST DEVICE:
if exist Host\host_device\build\zephyr\zephyr.hex (
    echo   - compiled_code\%host_output_name%
    for %%F in (Host\host_device\build\zephyr\zephyr.hex) do echo     Size: %%~zF bytes
) else (
    echo   - Host hex file (NOT FOUND)
)

echo.
echo MIPE DEVICE:
if exist Mipe\build\zephyr\zephyr.hex (
    echo   - compiled_code\%mipe_output_name%
    for %%F in (Mipe\build\zephyr\zephyr.hex) do echo     Size: %%~zF bytes
) else (
    echo   - Mipe hex file (NOT FOUND)
)

echo.
echo ========================================
echo Build Summary
echo ========================================
echo.
echo To flash the Host device:
echo   nrfjprog --program compiled_code\%host_output_name% --chiperase --verify -r
echo.
echo To flash the Mipe device:
echo   nrfjprog --program compiled_code\%mipe_output_name% --chiperase --verify -r
echo.
echo Or use nRF Connect Programmer app for both devices
echo.

pause
