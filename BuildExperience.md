# Build Experience and Lessons Learned

## Project: nRF54L15DK Zephyr Development
**Date:** August 18, 2025  
**SDK Version:** nRF Connect SDK v3.1.0  
**Board:** nRF54L15DK (nrf54l15/cpuapp)

---

## 🚨 Critical Issues Encountered & Solutions

### 1. Directory Path with Spaces
**Issue:** CMake fails to parse paths containing spaces (e.g., "Development (1)")
```
CMake Error: File not found: D:/Development
```

**Solution:** 
- Build in paths without spaces
- Rename folders using underscores or hyphens
- Example: `Development_1` instead of `Development (1)`

**Permanent Fix:** 
```powershell
# Good paths:
C:\ncs\projects\my_project
D:\Development_nRF\project

# Bad paths:
D:\Development (1)\project
C:\My Projects\embedded
```

---

## 🛠️ Build System Configuration

### Environment Variables Required
```powershell
# Set these before building
$env:ZEPHYR_BASE = "C:/ncs/v3.1.0/zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:/ncs/toolchains/b8b84efebd/opt/zephyr-sdk"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"

# Add to PATH
$env:PATH = "C:\ncs\toolchains\b8b84efebd\opt\bin;C:\ncs\toolchains\b8b84efebd\opt\bin\Scripts;" + $env:PATH
```

### Build Commands That Work
```powershell
# Navigate to project
cd "path\to\project"

# Configure with CMake
cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp

# Build
ninja -C build

# Clean build
rm -r build
cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp --pristine
```

---

## 📁 VSCode Configuration Files

### Essential Files Created
1. **`.vscode/c_cpp_properties.json`** - IntelliSense configuration
2. **`.vscode/settings.json`** - CMake and build settings

### Key Include Paths for IntelliSense
- `${env:ZEPHYR_BASE}/include`
- `${workspaceFolder}/build/zephyr/include/generated`
- Multiple nRF Connect SDK version paths as fallbacks

**Tip:** Copy the `.vscode` folder to new projects as a template

---

## 🎯 West Tool Issues

### Problem
West build command not available directly:
```
west: unknown command "build"
```

### Root Cause
- West workspace not initialized in the project directory
- nRF Connect SDK v3.1.0 uses a different workspace structure

### Solution
Use CMake directly instead of west build:
```powershell
# Instead of: west build -b board_name
# Use: cmake + ninja
cmake -B build -G Ninja -DBOARD=board_name
ninja -C build
```

---

## 📊 Build Results Summary

### Memory Usage (Simple Blinky)
- **FLASH:** 46,152 B / 1,428 KB (3.16%)
- **RAM:** 8,528 B / 188 KB (4.43%)

### Generated Files
- `zephyr.hex` - For flashing (nrfjprog, Programmer)
- `zephyr.bin` - Binary format
- `zephyr.elf` - Debug symbols

---

## 🚀 Quick Setup for Next Project

### 1. Create Project Structure
```powershell
# Use clean path without spaces
mkdir C:\ncs\projects\new_project
cd C:\ncs\projects\new_project

# Create source directory
mkdir src
```

### 2. Copy Configuration
```powershell
# Copy VSCode settings from working project
Copy-Item "path\to\working\.vscode" -Destination ".\" -Recurse

# Copy CMakeLists.txt template
Copy-Item "path\to\working\CMakeLists.txt" -Destination ".\"
```

### 3. Build Script Template
Create `build.ps1`:
```powershell
# Set environment
$env:ZEPHYR_BASE = "C:/ncs/v3.1.0/zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:/ncs/toolchains/b8b84efebd/opt/zephyr-sdk"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
$env:PATH = "C:\ncs\toolchains\b8b84efebd\opt\bin;C:\ncs\toolchains\b8b84efebd\opt\bin\Scripts;" + $env:PATH

# Clean and build
if (Test-Path "build") { Remove-Item -Recurse -Force "build" }
cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp
ninja -C build
```

---

## ⚠️ Common Pitfalls to Avoid

1. **Don't use spaces in project paths**
2. **Don't forget to set environment variables**
3. **Don't mix SDK versions** (stick with one nRF Connect SDK version)
4. **Don't use `west build` without proper workspace init**
5. **Don't forget the board variant** (`/nrf54l15/cpuapp` part)

---

## 🔧 Troubleshooting Checklist

If build fails:
- [ ] Check path has no spaces
- [ ] Verify environment variables are set
- [ ] Confirm nRF Connect SDK is installed
- [ ] Check CMakeLists.txt has correct `find_package(Zephyr)`
- [ ] Verify board name is correct
- [ ] Try clean build (delete build folder)
- [ ] Check Windows Defender isn't blocking tools

---

## 📝 Notes for Future Development

### Toolchain Versions
- **nRF Connect SDK:** v3.1.0
- **Zephyr:** v4.1.99
- **West:** v1.4.0
- **CMake:** v3.21.0
- **Python:** v3.12.4
- **Toolchain ID:** b8b84efebd

### Board Configuration
- **Board:** nrf54l15dk
- **SoC:** nRF54L15
- **CPU:** Application core (cpuapp)
- **Architecture:** ARM Cortex-M33

### Successful Build Command Sequence
```powershell
# This exact sequence worked:
cd "C:\Users\under\Desktop\simple_blinky"  # No spaces in path!
cmake -B build -G Ninja -DBOARD=nrf54l15dk/nrf54l15/cpuapp
ninja -C build
```

---

## 💡 Best Practices

1. **Always build in clean paths** (no spaces or special characters)
2. **Keep a working .vscode template** for new projects
3. **Document your build environment** (SDK versions, toolchain)
4. **Use build scripts** to ensure consistent environment
5. **Test build immediately** after project creation
6. **Keep this document updated** with new experiences

---

## 📚 Useful References

- [nRF Connect SDK Documentation](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/index.html)
- [Zephyr Project Documentation](https://docs.zephyrproject.org/)
- [nRF54L15DK Product Page](https://www.nordicsemi.com/Products/Development-hardware/nRF54L15-DK)

---

## 🔵 Nordic BLE Scan Library Issue

### Problem
```
fatal error: zephyr/bluetooth/scan.h: No such file or directory
```

### Root Cause
- Nordic's proprietary BLE scan library (`CONFIG_BT_SCAN`) is not available in all configurations
- The `<zephyr/bluetooth/scan.h>` header is Nordic-specific

### Solution
Use standard Zephyr BLE APIs instead:
```c
// Instead of Nordic scan library:
#include <zephyr/bluetooth/scan.h>
bt_scan_init(&scan_init);
bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);

// Use standard Zephyr:
#include <zephyr/bluetooth/bluetooth.h>
struct bt_le_scan_param scan_param = {
    .type = BT_LE_SCAN_TYPE_ACTIVE,
    .options = BT_LE_SCAN_OPT_NONE,
    .interval = BT_GAP_SCAN_FAST_INTERVAL,
    .window = BT_GAP_SCAN_FAST_WINDOW,
};
bt_le_scan_start(&scan_param, device_found);
```

---

## 🔴 BLE Connection Callback Issues

### Problem
```
error: 'const struct bt_conn_cb' has no member named 'security_changed'
```

### Solution
The `security_changed` callback may not be available in all configurations. Remove it:
```c
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    // .security_changed = security_changed,  // Remove if not supported
};
```

---

## 📋 Function Signature Mismatches

### Common Issues & Fixes

**Logger initialization:**
```c
// Wrong: logger_init();
// Right: logger_init(LOGGER_LEVEL_INFO);
```

**Button handler initialization:**
```c
// Wrong: button_handler_init();
// Right: button_handler_init(callback_function);
// Or:    button_handler_init(NULL);  // If no callback needed
```

**Cache enable functions:**
```c
// Wrong: icache_enable(); dcache_enable();
// Right: Remove - caches are handled by Zephyr automatically
```

---

## 🛠️ Host Device Build Success

### Working Configuration (August 18, 2025)
- **Project:** SinglePing Host Device
- **Board:** nrf54l15dk/nrf54l15/cpuapp
- **Build Time:** ~2 minutes
- **Output Size:** 730 KB hex file
- **Key Dependencies:**
  - BLE Central role
  - GATT Client
  - GPIO for LEDs
  - Display support (CONFIG_DISPLAY=y)
  - Networking for timestamps

### Files Created for Successful Build
- `ble_central.c` - Standard Zephyr BLE implementation
- `button_handler.c/h` - Complete button handling with callbacks
- `statistics.c` - Statistics calculations
- `logger.c/h` - Custom logger with proper enum definitions
- `timer_utils.c` - Timer utilities

---

## 🔄 BLE Dual-Role Configuration Success

### Achievement (August 20, 2025)
Successfully implemented simultaneous BLE connections with Host acting as both Peripheral and Central.

### Key Configuration Changes
```conf
# In prj.conf - Enable dual-role operation
CONFIG_BT_MAX_CONN=8        # Was 1, now supports multiple connections
CONFIG_BT_CENTRAL=y         # Enable Central role for Mipe connections
CONFIG_BT_OBSERVER=y        # Enable scanning capability
CONFIG_BT_PERIPHERAL=y     # Keep Peripheral role for MotoApp
```

### Critical Implementation Lessons

**1. Role Filtering in Callbacks**
```c
// IMPORTANT: Filter connections by role in callbacks
static void connected_cb(struct bt_conn *conn, uint8_t err) {
    struct bt_conn_info info;
    bt_conn_get_info(conn, &info);
    
    // Only handle appropriate role connections
    if (info.role != BT_CONN_ROLE_PERIPHERAL) {
        return;  // Not a connection from MotoApp
    }
    // Handle MotoApp connection...
}
```

**2. Device Name Prefix Matching**
```c
// Be precise with device name prefixes
#define MIPE_DEVICE_PREFIX "MIPE"     // NOT "MIPE_" 
// Actual device advertises as "MIPE" not "MIPE_"
```

**3. Reconnection Logic**
```c
// Add delay before resuming scan after disconnection
static void disconnected_cb(struct bt_conn *conn, uint8_t reason) {
    k_sleep(K_MSEC(100));  // Let BLE stack settle
    ble_central_start_scan();  // Then resume scanning
}
```

### Build Performance Improvements
- Clean builds now completing consistently
- No more cryptic CMake errors
- PowerShell build script with `--pristine` flag works reliably
- Memory usage stable even with dual-role enabled

### Known Issue to Address
**Data Pipeline Break:** When Mipe connects, data streaming to MotoApp stops because:
- Host stops sending simulated data when Mipe connects
- GATT subscription to Mipe's RSSI characteristic not yet implemented
- Solution: Implement proper GATT service discovery and notification subscription

---

## 📈 Build Success Rate Improvement

### Before (August 18)
- Multiple build failures due to path issues
- Missing file errors
- Function signature mismatches
- ~50% success rate

### After (August 20)
- Smooth, consistent builds
- Clear error messages when issues occur
- Build script automation working
- ~95% success rate

### Key Factors for Improvement
1. **Consistent environment setup** via PowerShell scripts
2. **Clean build practice** with `--pristine` flag
3. **Proper path management** (no spaces)
4. **Template reuse** from working projects

---

*Last Updated: August 20, 2025*  
*Next Review: When implementing GATT subscription for Mipe RSSI data*
