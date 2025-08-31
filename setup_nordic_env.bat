@echo off
REM Nordic Connect SDK Environment Setup Script
REM Run this script before building your Zephyr projects

echo Setting up Nordic Connect SDK environment variables...

REM Set Zephyr environment variables
set ZEPHYR_BASE=C:\ncs\v3.1.0\zephyr
set ZEPHYR_TOOLCHAIN_VARIANT=zephyr
set ZEPHYR_SDK_INSTALL_DIR=C:\ncs\toolchains\b8b84efebd\opt\zephyr-sdk

REM Add Nordic toolchain to PATH
set PATH=C:\ncs\toolchains\b8b84efebd\opt\bin;%PATH%

REM Verify the setup
echo.
echo Environment variables set:
echo ZEPHYR_BASE=%ZEPHYR_BASE%
echo ZEPHYR_TOOLCHAIN_VARIANT=%ZEPHYR_TOOLCHAIN_VARIANT%
echo ZEPHYR_SDK_INSTALL_DIR=%ZEPHYR_SDK_INSTALL_DIR%
echo.
echo Nordic toolchain added to PATH
echo.
echo You can now build your Zephyr projects!
echo.
pause

