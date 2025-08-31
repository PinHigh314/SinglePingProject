# Nordic Connect SDK Environment Setup

This project uses Nordic Connect SDK (NCS) v3.1.0 with Zephyr RTOS for the nRF54L15DK development board.

## Environment Variables Required

The following environment variables must be set for successful builds:

- `ZEPHYR_BASE`: Points to the Zephyr installation bundled with NCS
- `ZEPHYR_TOOLCHAIN_VARIANT`: Specifies the toolchain variant
- `ZEPHYR_SDK_INSTALL_DIR`: Points to the Zephyr SDK installation
- `PATH`: Must include the Nordic toolchain binaries

## Quick Setup

### Option 1: Run Setup Script (Recommended for Development)

**Windows Command Prompt:**
```cmd
setup_nordic_env.bat
```

**PowerShell:**
```powershell
.\setup_nordic_env.ps1
```

### Option 2: Manual Setup

Set these environment variables manually:

```cmd
set ZEPHYR_BASE=C:\ncs\v3.1.0\zephyr
set ZEPHYR_TOOLCHAIN_VARIANT=zephyr
set ZEPHYR_SDK_INSTALL_DIR=C:\ncs\toolchains\b8b84efebd\opt\zephyr-sdk
set PATH=C:\ncs\toolchains\b8b84efebd\opt\bin;%PATH%
```

## Verification

After setting the environment variables, verify the setup:

1. **Check west workspace:**
   ```cmd
   cd Host/host_device
   west list
   ```

2. **Verify toolchain:**
   ```cmd
   "C:\ncs\toolchains\b8b84efebd\opt\zephyr-sdk\arm-zephyr-eabi\bin\arm-zephyr-eabi-gcc.exe" --version
   ```

3. **Test build:**
   ```cmd
   cd Host/host_device
   west build
   ```

## Current Configuration

- **NCS Version**: v3.1.0
- **Zephyr Version**: ncs-v3.1.0 (bundled with NCS)
- **Target Board**: nrf54l15dk/nrf54l15/cpuapp
- **Toolchain**: Zephyr SDK 0.17.0 with GCC 12.2.0

## Troubleshooting

### Common Issues

1. **"no west workspace found"**
   - Ensure `ZEPHYR_BASE` is set correctly
   - Run the setup script before building

2. **"cmake: command not found"**
   - Ensure Nordic toolchain is in PATH
   - Verify `ZEPHYR_SDK_INSTALL_DIR` is set

3. **Build failures with missing Zephyr components**
   - Clean build directory and rebuild
   - Ensure all environment variables are set

### Reset Environment

If you encounter issues, reset your environment:

```cmd
set ZEPHYR_BASE=
set ZEPHYR_TOOLCHAIN_VARIANT=
set ZEPHYR_SDK_INSTALL_DIR=
```

Then run the setup script again.

## Permanent Setup (Optional)

To avoid running the setup script each time:

1. **System Environment Variables:**
   - Open System Properties → Environment Variables
   - Add the variables to your user or system environment
   - Restart your terminal/IDE

2. **User Profile:**
   - Add the setup commands to your `.bashrc`, `.profile`, or equivalent
   - This will automatically set the variables when you open a new terminal

## Notes

- The setup scripts are designed for Windows
- For Linux/macOS, modify the paths accordingly
- Always run the setup script in a new terminal session
- The environment variables are session-specific unless set permanently

