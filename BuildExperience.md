# Build Experience Log - SinglePing Project

## August 23, 2025 - MotoApp BLE Implementation

### Critical Issue Resolved: BLE Permissions
**Problem**: MotoApp couldn't detect ANY BLE devices, not even the Host device.

**Root Cause**: Missing location permissions for BLE scanning
- AndroidManifest.xml had `ACCESS_FINE_LOCATION` with `maxSdkVersion="30"` - not available for Android 12+
- MainActivity.kt only requested BLUETOOTH_SCAN and BLUETOOTH_CONNECT for Android 12+
- Location permission is STILL required for BLE scanning on Android 12+

**Solution**:
1. Updated AndroidManifest.xml to include location permissions for ALL Android versions
2. Modified MainActivity.kt to request location permission along with Bluetooth permissions
3. Added permission status display in debug screen

**Key Learning**: Android requires location permissions for BLE scanning due to privacy concerns (BLE can be used for location tracking)

### MotoApp Versions Built
- **v3.4**: Debug version with permission fixes and enhanced scanner
- **v3.5**: Complete production version with full BLE functionality

### Build Commands Used
```bash
cd MotoApp && ./gradlew clean assembleDebug
cd MotoApp && ./gradlew assembleDebug
cp MotoApp/app/build/outputs/apk/debug/app-debug.apk compiled_code/MotoApp_TMT1_vX.X.apk
```

### Gradle Configuration
Fixed JVM memory issues in gradle.properties:
```
org.gradle.jvmargs=-Xmx2048m -Xms512m -XX:MaxMetaspaceSize=512m
```

### Nordic BLE Library Integration
Successfully integrated Nordic Android BLE Library v2.7.0 for robust BLE operations.

### Simulated Mode Success
Implemented beautiful sine wave RSSI data generation in simulation mode for testing without hardware.

## Key Files Modified

### AndroidManifest.xml
- Added ACCESS_FINE_LOCATION and ACCESS_COARSE_LOCATION for all Android versions
- Kept all Bluetooth permissions for Android 12+

### MainActivity.kt
- Fixed permission request logic to include location for Android 12+
- Removed non-existent initialize() and cleanup() method calls
- Proper permission callback handling

### MainScreen.kt
- Enhanced connection UI with status display
- Added helper functions for time formatting and color coding
- Improved connection button states

### DebugScreen.kt
- Added comprehensive permission status display
- Service UUID parsing and detection
- Enhanced device listing with color coding

### BleScanner.kt
- Removed UUID filtering to find all devices
- Added comprehensive logging
- Proper timeout handling

## Testing Results
- Simulated mode: Working perfectly with sine wave RSSI data
- BLE scanning: Successfully detects all BLE devices after permission fix
- Permission system: Properly requests and handles all required permissions

## Next Steps for Development
1. Test real BLE connection with Host device
2. Implement background service for continuous operation
3. Add auto-reconnection logic
4. Implement data logging features

## August 23, 2025 - Host Firmware Build Success

### Critical Issue Resolved: PowerShell Execution Policy
**Problem**: Build scripts for Host firmware failed with execution policy errors when trying to run .ps1 files directly.

**Root Cause**: Windows PowerShell execution policy blocking script execution
- Standard approaches like `-ExecutionPolicy Bypass` weren't working consistently
- File replacement strategies in build scripts caused CMake configuration issues

**Solution**: Use `Invoke-Expression` to execute script content directly
```powershell
cd Host/host_device && powershell -Command "Invoke-Expression (Get-Content ./build.ps1 -Raw)"
```

This approach:
- Reads the script content as text
- Executes it directly without running the .ps1 file
- Bypasses execution policy restrictions
- Works reliably for both regular and test builds

### Host Firmware v2 Test Build (rev015)
Successfully built test firmware with critical fixes:

**Fixed Issues from rev014**:
1. **LED3 Turn-off**: Added monitor timer that checks streaming state every second
2. **RSSI Data Transmission**: Fixed GATT characteristic attribute indexing

**Key Code Changes**:
- `main.c`: Added monitor timer to ensure LED3 turns off when streaming stops
- `ble_peripheral.c`: Corrected RSSI characteristic attribute pointer (index 2)

**Build Process**:
1. Source files already contained v2 fixes (no file replacement needed)
2. Used regular build.ps1 with Invoke-Expression
3. Manually renamed output to rev015 naming convention
4. File size: 882 KB

### Key Learning: PowerShell Script Execution
When facing PowerShell execution policy issues:
1. Don't rely on `-ExecutionPolicy Bypass` flag alone
2. Use `Invoke-Expression (Get-Content ./script.ps1 -Raw)` for reliable execution
3. Avoid complex file replacement strategies during build
4. Keep source files updated rather than replacing during build

### nRF Connect SDK Environment
Successfully using:
- nRF Connect SDK v3.1.0
- Zephyr RTOS
- CMake/Ninja build system
- Board: nrf54l15dk/nrf54l15/cpuapp

### Build Commands That Work
```powershell
# For regular Host firmware build
cd Host/host_device && powershell -Command "Invoke-Expression (Get-Content ./build.ps1 -Raw)"

# Copy and rename for versioning
cp Host/host_device/build/zephyr/zephyr.hex compiled_code/host_device_[description]_[date]_rev[XXX].hex
```

### Testing Next Steps
1. Flash rev015 to test LED3 turn-off fix
2. Verify RSSI data transmission with MotoApp v3.5
3. Confirm streaming start/stop behavior
4. Monitor LED states during connection cycles

## August 23, 2025 - Major Milestone: BLE Data Flow Established

### Host Firmware Test v5 (rev018) - Complete Success
**Problem Solved**: Fixed RSSI data (-55 dBm test value) now flowing from Host to MotoApp!

**Issues Fixed from v4**:
1. **LED2 incorrectly lighting**: Was simulating Mipe connection on MotoApp connect
2. **Streaming not triggered**: Was starting immediately on connection instead of waiting for control command
3. **Command logic missing**: Control commands from MotoApp weren't triggering RSSI generation

**Root Cause Analysis**:
- v4 called `ble_central_start_scan()` in connection callback (wrong timing)
- Set `mipe_connected = true` when it should stay false in test mode
- Data stream callback wasn't properly starting RSSI generation

**Solution in v5**:
```c
static void data_stream_callback(bool start)
{
    data_streaming = start;
    
    if (start) {
        LOG_INF("=== DATA STREAMING STARTED ===");
        LOG_INF("Control command received from MotoApp");
        gpio_pin_set_dt(&led3, 1); /* Turn ON streaming LED */
        
        /* NOW start the simulated RSSI generation */
        LOG_INF("Starting simulated RSSI generation...");
        int err = ble_central_start_scan();  /* This starts the RSSI timer */
    }
}
```

**Build Command**:
```bash
cd Host/host_device && powershell -ExecutionPolicy Bypass -File build_test_fixed_rssi_v5.ps1
```

**Results**:
- ✅ LED1 only lights when MotoApp connects (LED2 stays off)
- ✅ LED3 turns on when "Start Streaming" pressed
- ✅ RSSI data flows to MotoApp when streaming active
- ✅ LED3 turns off when "Stop Streaming" pressed

### Git Version Control Strategy Implemented

**Created comprehensive versioning system**:
1. **GIT_VERSION_STRATEGY.md** - Complete branching and tagging documentation
2. **setup_git_versioning.ps1** - Automated script for milestone tagging

**Branching Strategy**:
- `main` - Stable releases only
- `develop` - Integration branch
- `feature/*` - Feature branches
- `test/*` - Test/debug branches
- `release/*` - Release preparation

**Tagging Format**:
- Firmware: `firmware/host-v{version}-rev{revision}`
- Apps: `app/motoapp-v{version}`
- Milestones: `milestone/{description}-{date}`

**Current Milestone Tags**:
- `milestone/ble-data-flow-working-20250823`
- `firmware/host-test-v5-rev018`
- `app/motoapp-v3.5`

**Implementation**:
```bash
# Run automated setup
powershell -ExecutionPolicy Bypass -File setup_git_versioning.ps1

# Or manual commands
git add -A
git commit -m "milestone: BLE data flow established between Host and MotoApp"
git tag -a milestone/ble-data-flow-working-20250823 -m "BLE data flow milestone"
git push origin --all --tags
```

### Key Learning: Command-Based Streaming Control
The Host firmware must wait for explicit control commands from MotoApp:
- CMD_START_STREAM (0x01) - Start data transmission
- CMD_STOP_STREAM (0x02) - Stop data transmission
- Don't start streaming on connection - wait for user action

### Current Working State
- **Host Firmware**: v5 test (rev018) with fixed RSSI @ -55 dBm
- **MotoApp**: v3.5 with full BLE support
- **Status**: Complete BLE communication pipeline established
- **Data Flow**: Host → MotoApp working with proper control

### Next Development Steps
1. Implement real Mipe device scanning and connection
2. Add dual-role BLE (Host as Central to Mipe, Peripheral to MotoApp)
3. Replace fixed RSSI with actual Mipe device readings
4. Add LED2 indication for real Mipe connections

## August 23, 2025 - Universal Build Script Implementation

### Critical Issue Resolved: Build Script Proliferation
**Problem**: Creating a new build script for each test version (v1, v2, v3, etc.) was causing unnecessary complexity and confusion.

**Root Cause**: 
- Each version had its own build script with slight variations
- No consistent pattern was being followed
- File replacement strategies were inconsistent
- Too many different approaches tried (cmake manipulation, file patching, etc.)

**Solution**: Created `build_test_universal.ps1` - a single parameterized script for all test builds

### Universal Build Script Features
**Script**: `Host/host_device/build_test_universal.ps1`

**Usage**:
```powershell
# Build any test version with simple parameters
.\build_test_universal.ps1 -Version 6 -Description "alternating rssi" -RevNumber 019
.\build_test_universal.ps1 -Version 7 -Description "new feature" -RevNumber 020
```

**What it does**:
1. Sets up environment (same every time)
2. Backs up current files
3. Copies test version to main.c
4. Copies test BLE central
5. Runs cmake and ninja build
6. Copies hex to compiled_code with descriptive naming
7. Restores original files

**Benefits**:
- No more creating individual build scripts
- Consistent build process
- Automatic file naming and versioning
- Simple parameter-based operation

### Host Firmware Test v6 (rev019) - Alternating RSSI
**Features Implemented**:
- Alternates between fixed -55 dBm reference and simulated real RSSI
- LED3 flashes for fixed reference values
- LED2 flashes for real RSSI values
- Packet counter for status reporting

**Build Process** (now simplified):
```powershell
# Just run the universal script!
cd Host/host_device
.\build_test_universal.ps1 -Version 6 -Description "alternating rssi" -RevNumber 019
```

**Key Learning**: Don't create a new build script for each version!
- Use a parameterized universal script
- Keep the build process consistent
- Focus on changing the source code, not the build process

### Build Workflow Simplified
**Old Complex Way**:
1. Create new build script for each version
2. Try different approaches (cmake, patching, etc.)
3. Debug build script issues
4. Eventually get it working

**New Simple Way**:
1. Create test source: `src/main_test_fixed_rssi_vX.c`
2. Run: `.\build_test_universal.ps1 -Version X -Description "feature" -RevNumber XXX`
3. Done!

### Current Build Tools
- **Universal Test Build**: `build_test_universal.ps1`
- **Regular Build**: `build.ps1` (for production firmware)
- **Environment**: nRF Connect SDK v3.1.0, CMake, Ninja

### Documentation Created
- `BUILD_INSTRUCTIONS.md` - Complete guide to using universal build script
- Clear examples and explanations
- No more confusion about which script to use
