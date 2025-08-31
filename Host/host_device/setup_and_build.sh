#!/bin/bash

# Set up Nordic Connect SDK environment variables
export ZEPHYR_BASE="C:/ncs/v3.1.0/zephyr"
export ZEPHYR_TOOLCHAIN_VARIANT="zephyr"
export ZEPHYR_SDK_INSTALL_DIR="C:/ncs/toolchains/b8b84efebd/opt/zephyr-sdk"
export PATH="C:/ncs/toolchains/b8b84efebd/opt/bin:$PATH"

# Verify environment
echo "Environment variables set:"
echo "ZEPHYR_BASE: $ZEPHYR_BASE"
echo "ZEPHYR_TOOLCHAIN_VARIANT: $ZEPHYR_TOOLCHAIN_VARIANT"
echo "ZEPHYR_SDK_INSTALL_DIR: $ZEPHYR_SDK_INSTALL_DIR"
echo "PATH includes Nordic toolchain: $PATH" | grep -q "b8b84efebd" && echo "✓ Nordic toolchain in PATH" || echo "✗ Nordic toolchain NOT in PATH"

# Check if cmake is available
if command -v cmake &> /dev/null; then
    echo "✓ CMake found: $(cmake --version | head -1)"
else
    echo "✗ CMake not found in PATH"
    echo "Trying full path..."
    if [ -f "C:/ncs/toolchains/b8b84efebd/opt/bin/cmake.exe" ]; then
        echo "✓ CMake found at full path"
        export CMAKE_COMMAND="C:/ncs/toolchains/b8b84efebd/opt/bin/cmake.exe"
    else
        echo "✗ CMake not found at full path either"
        exit 1
    fi
fi

echo ""
echo "Starting west build..."
west build --board nrf54l15dk/nrf54l15/cpuapp

