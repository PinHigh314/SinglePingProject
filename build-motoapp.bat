@echo off
echo ========================================
echo MotoApp Build Script
echo ========================================
echo.

REM Change to MotoApp directory
cd /d "%~dp0MotoApp"

REM Check if gradlew exists
if not exist "gradlew.bat" (
    echo ERROR: gradlew.bat not found in MotoApp directory
    echo Please ensure you're running this from the SinglePingProject root
    pause
    exit /b 1
)

echo Cleaning previous build...
call gradlew.bat clean

echo.
echo Building MotoApp...
call gradlew.bat assembleDebug

REM Check if build was successful
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

REM Check if APK was created
if not exist "app\build\outputs\apk\debug\app-debug.apk" (
    echo.
    echo ERROR: APK file not found!
    pause
    exit /b 1
)

echo.
echo Build successful!

REM Get current date and time for versioning
for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value') do set "dt=%%a"
set "YY=%dt:~2,2%"
set "MM=%dt:~4,2%"
set "DD=%dt:~6,2%"
set "HH=%dt:~8,2%"
set "Min=%dt:~10,2%"

REM Create timestamp
set "timestamp=%YY%%MM%%DD%_%HH%%Min%"

REM Get next version number
set /a version=36
:findversion
if exist "..\compiled_code\MotoApp_v3.%version%_*.apk" (
    set /a version+=1
    goto findversion
)

REM Copy APK with version and timestamp
set "filename=MotoApp_v3.%version%_%timestamp%.apk"
echo.
echo Copying APK to compiled_code\%filename%...
copy "app\build\outputs\apk\debug\app-debug.apk" "..\compiled_code\%filename%"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo BUILD COMPLETE!
    echo ========================================
    echo APK Location: compiled_code\%filename%
    
    REM Get file size
    for %%A in ("..\compiled_code\%filename%") do set size=%%~zA
    set /a sizeMB=%size%/1048576
    echo File Size: %sizeMB% MB
    
    echo.
    echo MotoApp Features:
    echo - BLE connectivity with Host device
    echo - Real-time RSSI monitoring
    echo - Distance calculation
    echo - Debug screen for diagnostics
    echo - Auto-reconnection support
    echo ========================================
) else (
    echo.
    echo ERROR: Failed to copy APK file
)

REM Return to original directory
cd /d "%~dp0"

echo.
pause
