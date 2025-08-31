@echo off
echo Running Host Device Build with west build...
echo.

REM Run PowerShell with execution policy bypass
powershell -ExecutionPolicy Bypass -File "build_west.ps1"

echo.
echo Build script completed.
pause

