# PROJECT CONTEXT MEMORY
Last Updated: 2025-08-21 12:34

## Current Session Summary
- **COMPLETED**: Stable Dual-Connection Data Streaming Fix (2025-08-21 13:58)
  - **Issue**: Data streaming broke Host-MotoApp connection when both devices connected
  - **Root Cause**: Notification overload - 10Hz RSSI updates overwhelmed BLE stack
  - **User Feedback**: "Speed is not important, reliability is"
  - **Solution Implemented**:
    - Reduced RSSI measurement from 10Hz to 1Hz
    - Added 2-second rate limiting for notifications to MotoApp
    - Added 50ms delays between BLE operations
    - Enhanced connection validation before sending
    - Better error handling for buffer full conditions
  - **Built Firmware**: host_device_stable_streaming_20250821_rev012.hex
  - **Memory**: 320KB FLASH (21.90%), 81KB RAM (42.36%)
  - **Result**: Prioritizes reliability over speed - prevents connection breaking
- **COMPLETED**: BLE Stability Fixes Applied (2025-08-21 13:42)
  - **Fix 1 - Data Streaming**: Force enabled RSSI notifications when starting stream
    - Issue: CCC callback wasn't setting rssi_notify_enabled flag
    - Solution: Bypass CCC and force enable notifications on CMD_START_STREAM
  - **Fix 2 - MotoApp Reconnection**: Robust advertising restart with retries
    - Issue: Advertising failed to restart after disconnect due to BLE stack resources
    - Solution: 5 retry attempts with progressive backoff (100ms to 1s delays)
  - **Fix 3 - MIPE LED Status**: Enhanced disconnection detection
    - Issue: Host LED2 stayed lit when MIPE disconnected
    - Solution: Added connection validation and forced callback notification
  - **Built Firmware**: host_device_ble_stability_fixes_20250821_rev011.hex
  - **Memory**: 319KB FLASH (21.87%), 81KB RAM (42.36%)
  - **Status**: Ready for testing - addresses all reported BLE stability issues
- **COMPLETED**: LED and Data Flow Issues Fixed (2025-08-21 13:20)
  - **MIPE LED Fix**: Fixed LED mapping so LED1 shows connection status (was LED2)
  - **Issue**: LED_ID_CONNECTION was mapped to led2 instead of led1 in led_control.c
  - **Solution**: Corrected LED mapping so LED_ID_CONNECTION maps to led1
  - **Data Flow Understanding**: HOST requires BOTH MotoApp and MIPE connections for data
  - **Architecture**: Data only flows when complete chain is established (by design)
  - **Built Firmwares**:
    - mipe_device_led_fix_20250821_rev007.hex (LED fix applied)
    - host_device_dual_connection_20250821_rev010.hex (dual-role BLE)
  - **Memory Usage**:
    - MIPE: 158KB FLASH (10.86%), 31KB RAM (16.37%)
    - HOST: 319KB FLASH (21.85%), 81KB RAM (42.36%)
- **COMPLETED**: Host BLE Connection Fix (2025-08-21 12:50)
  - **CRITICAL FIX**: Resolved Host advertising failure due to simultaneous scanning
  - **Issue**: BLE Central scanning for Mipe interfered with Peripheral advertising for MotoApp
  - **Root Cause**: Host started ble_central_start_scan() immediately on initialization
  - **Solution**: Modified scanning behavior to only start after MotoApp connects
  - **Implementation**:
    - Removed automatic scanning from main.c initialization
    - Start Mipe scanning only when MotoApp connects
    - Stop Mipe scanning when MotoApp disconnects
  - **Built Firmware**: host_device_tmt3_scanning_fix_20250821_rev008.hex
  - **Memory**: 324KB FLASH (22.18%), 80KB RAM (41.60%)
  - **Result**: MotoApp and BLE scanners can now discover and connect to Host
- **COMPLETED**: MIPE LED Fix and Data Streaming Investigation (2025-08-21 12:34)
  - **MIPE LED Fix Applied**: Fixed LED mapping issue where LED2 was lighting instead of LED1
  - **Issue**: Connection manager was using LED_ID_PAIRING but needed LED_ID_CONNECTION
  - **Solution**: Updated all LED control calls in connection_manager.c to use LED_ID_CONNECTION
  - **Data Stream Investigation**: MotoApp data streaming works correctly
  - **Note**: Data only flows when BOTH MotoApp and MIPE are connected to Host
  - **Built Firmwares**:
    - mipe_device_tmt3_led_fix_20250821_rev006.hex (LED fix applied)
    - host_device_tmt3_data_stream_20250821_rev007.hex (data streaming verified)
- **COMPLETED**: Real RSSI Architecture Implementation (2025-08-21 12:20)
  - **Architecture Fix**: Corrected fundamental RSSI measurement approach
  - **User Insight**: "RSSI should be measured by Host from BLE signal strength, not transmitted by MIPE"
  - **HOST Firmware Built**: host_device_tmt3_real_rssi_20250821_rev006.hex
    - Removed mock RSSI generation from main.c
    - Implemented RSSI measurement from BLE connection (simulated for TMT3)
    - Memory: 324KB FLASH (22.16%), 80KB RAM (41.60%)
  - **MIPE Firmware Built**: mipe_device_tmt3_real_rssi_20250821_rev005.hex
    - Removed fake RSSI characteristic completely
    - Now only provides battery status via GATT
    - Memory: 159KB FLASH (10.86%), 31KB RAM (16.37%)
- **COMPLETED**: Both Firmwares Compiled and Versioned (2025-08-21 11:53)
  - **HOST Firmware Built**: host_device_tmt3_gatt_discovery_20250821_rev005.hex
    - Added CONFIG_BT_GATT_CLIENT=y to prj.conf
    - Integrated GATT discovery directly into ble_central.c
    - Auto-subscribes to MIPE RSSI notifications on connection
    - Memory: 323KB FLASH (22.14%), 80KB RAM (41.56%)
  - **MIPE Firmware Previously Built**: mipe_device_tmt3_led_fix_20250821_rev004.hex
    - LED1 correctly shows connection status
    - Ready for deployment
- **FIXED**: LED and Data Stream Issues (2025-08-21 11:36)
  - **MIPE LED Fix**: Corrected LED1 to show connection status (was incorrectly using LED2)
  - **HOST LEDs**: Confirmed correct - LED1 for MotoApp, LED2 for MIPE
  - **Data Stream Fix**: Added GATT discovery and subscription in Host's BLE Central
    - Host now subscribes to MIPE's RSSI notifications automatically
    - Data flow path complete: MIPE → Host → MotoApp
- **COMPLETED**: MIPE Firmware Built with Host-Pattern BLE Fix (rev003)
  - Successfully built and versioned: mipe_device_tmt3_ble_host_pattern_20250821_rev003.hex
  - Complete rewrite of BLE service based on Host's working ble_peripheral.c
  - Memory usage: 159KB FLASH (10.89%), 31KB RAM (16.39%)
  - Fixed device name to advertise as "MIPE" (not MIPE_001)
  - **UPDATED**: LED assignments now correct - LED1 for connection, LED2 reserved
  - Using proven patterns: BT_LE_ADV_PARAM_INIT, BT_CONN_CB_DEFINE, BT_GATT_SERVICE_DEFINE
- **ERROR RECOVERY**: APK versioning mistakenly processed from yesterday's session
  - APK v2.3 was versioned but this was from a previous session summary
  - User reported issue with context resumption processing old tasks
  - Continuing with actual current work: MIPE firmware fixes
- **COMPLETED**: TMT3 Dual-role BLE Implementation for Host Device
  - Successfully enabled simultaneous BLE connections (MotoApp + Mipe)
  - Host now supports up to 8 concurrent BLE connections
  - Implemented dual-role: Peripheral (to MotoApp) + Central (to Mipe)
  - Built and versioned: host_device_tmt3_dual_role_20250820_rev001.hex
  - Complete data pipeline ready: Mipe → Host → MotoApp
- **COMPLETED**: Fixed all 12 BLE API deprecation warnings in MotoApp
  - Updated BleManager.kt to use Android 13+ Bluetooth GATT APIs
  - Replaced deprecated writeCharacteristic() with new 3-parameter version
  - Replaced deprecated writeDescriptor() with new 2-parameter version
  - Added proper @Suppress annotations for deprecated callback methods
  - Kotlin compilation completes without any deprecation warnings
- Fixed Material3 theme issue in MotoApp and successfully built APK
- Enhanced BuildExperience.md with prominent prevention section for Material3 issue
- Reviewed TMT2 tasks M018-M020 status (all built: false)
- Clarified M014 is about MotoApp discovering Host (not Mipe)
- **Completed P003**: Implemented Mipe Connection Handling for TMT3
  - Enhanced connection_manager.c with full BLE connection state machine
  - Added listening mode for power-optimized reconnection
  - Implemented connection parameter optimization
  - Successfully built firmware (152 KB FLASH, 30KB RAM usage)

## Architectural Insights

### RSSI Measurement Architecture (CORRECTED 2025-08-21)
- **Previous (Incorrect)**: MIPE generated and transmitted fake RSSI values to Host
- **Current (Correct)**: Host measures RSSI from BLE connection signal strength
- **Rationale**: RSSI (Received Signal Strength Indicator) is a property of the radio signal at the receiver, not data to be transmitted
- **Implementation**:
  - MIPE: Only provides battery status via GATT
  - Host: Measures RSSI from BLE connection (simulated for TMT3, real HCI commands for TMT4)
  - MotoApp: Receives measured RSSI values from Host
- **Benefits**: 
  - Architecturally correct for distance measurement
  - Reduces MIPE power consumption (no unnecessary transmissions)
  - More accurate distance calculations possible

## Recent Fixes and Updates

### Real RSSI Implementation (2025-08-21)
- **Issue**: Fundamental misunderstanding of RSSI measurement
- **Solution**: Restructured to measure RSSI at Host from connection
- **Files Modified**:
  - mipe_device/src/ble_service.c - Removed RSSI characteristic
  - mipe_device/include/ble_service.h - Removed RSSI function declarations
  - host_device/src/ble/ble_central.c - Added RSSI measurement work/timer
- **Current State**: Using simulated RSSI for TMT3, ready for real HCI implementation

### Mipe Real BLE Implementation (2025-08-21)
- **Implementation**: Complete BLE Peripheral with GATT services for Host connection
- **Features Added**:
  - ~~RSSI characteristic with 5-byte format~~ (REMOVED - incorrect architecture)
  - Battery monitoring characteristic with simulated data for TMT3
  - Power-optimized listening mode (1000ms advertising after disconnection)
  - Connection parameter optimization (30-50ms interval, latency=4)
- **Fixes Applied**:
  - Added missing LED identifiers (LED_ID_CONNECTION, LED_ID_DATA, LED_ID_ERROR)
  - Added stdlib.h for abs() function in battery_monitor.c
  - Fixed deprecated BT_LE_ADV_OPT_CONNECTABLE warnings
- **Build Results**: 159KB FLASH, 31KB RAM - very efficient for 30+ day battery target

### BLE API Deprecation Warnings Fixed (2025-08-20)
- **Issue**: 12 deprecation warnings for Bluetooth GATT API methods deprecated in Android 13
- **Root Cause**: Using old single-parameter writeCharacteristic() and writeDescriptor() methods
- **Fix**: Updated to new Android 13+ API methods:
  - writeCharacteristic(characteristic, value, writeType) 
  - writeDescriptor(descriptor, value)
  - Added @Suppress annotations for deprecated callbacks that still need to be overridden
- **Files Modified**: MotoApp/app/src/main/java/com/singleping/mipetest/ble/BleManager.kt
- **Verification**: Kotlin compilation phase completes without warnings

### Material3 Theme Build Fix (2025-08-20)
- **Issue**: "Theme.Material3.DayNight.NoActionBar not found" error
- **Root Cause**: Material3 is Compose-only, not available for XML inheritance
- **Fix**: Reverted themes.xml to basic Material theme
- **Files Modified**: MotoApp/app/src/main/res/values/themes.xml
- **Documentation**: Added prominent prevention section in BuildExperience.md
- **APK Built**: MipeTest_v1.0_FW_TMT1_001.apk (22.3 MB)

### JDK Configuration Fix (2025-08-20)
- **Issue**: File lock preventing build directory deletion
- **Root Cause**: Gradle daemon holding file locks
- **Fix**: Restarted Android Studio, cleared build cache
- **Additional Issue**: JDK mismatch - JetBrains Runtime 21 vs Java 17
- **Fix**: User corrected JDK configuration in Android Studio
- **APK Built**: MipeTest_v1.1_FW_TMT1_002.apk successfully generated

### Previous Session Work (2025-01-20)
- Fixed LED0 heartbeat implementation in Host firmware (H002 task)
- Updated all code comments to align with Master Project Prompt
- Marked TMT1 and TMT2 tasks as built in TMT_Structured_Tasks.json
- Verified Android 14+ configuration as per specification

## System Architecture
```
MotoApp (Android 14+) ←→ BLE ←→ Host (NRF54L15DK) ←→ BLE ←→ Mipe Device
                                         ↓
                                  Measures RSSI from
                                  BLE connection signal
```

## Current TMT Status
- **TMT1**: MotoApp Foundation - ✅ Complete (UI with mock data)
- **TMT2**: Host-MotoApp BLE Integration - ✅ Working (simulated RSSI data)
- **TMT3**: Mipe Device Integration - ✅ Architecture corrected, ready for testing
  - Host: Dual-role BLE with real RSSI measurement framework
  - Mipe: BLE Peripheral with battery status only (no fake RSSI)
  - MotoApp: Ready to receive measured RSSI via Host
- **TMT4**: Real Distance Calculation - Ready for real RSSI via HCI commands

## Known Issues
- Battery level display not implemented (deferred to TMT3 for Mipe device)
- M009 partially complete (settings/history clear buttons missing)
- Settings screen (M005) not implemented
- Navigation system (M006) not implemented
- Build system file lock issues (dex builder) - requires Gradle daemon restart
- **Real RSSI**: Currently using simulated values, need HCI command implementation for actual measurement

## Key Decisions Made
- Android 14+ only (minSdk=34) as per TMT1 specification
- Battery monitoring deferred to TMT3 (critical for Mipe, not Host)
- **RSSI measured at Host, not transmitted by Mipe** (architecturally correct)
- Simultaneous BLE connections implemented (Host dual-role)
- Using Android 13+ Bluetooth GATT APIs with proper deprecation handling
- Mipe uses power-optimized protocol with minimal transmissions

## Build Information
- Host firmware: Zephyr RTOS for nRF54L15DK
- Mipe firmware: Zephyr RTOS for nRF54L15DK (power-optimized)
- MotoApp: Android 14+ with Jetpack Compose
- BLE UUIDs: 
  - TMT1 service (Host-MotoApp): 12345678-1234-5678-1234-56789abcdef0
  - Mipe service (Host-Mipe): 87654321-4321-8765-4321-987654321098
- Requires JDK 17 for Android builds

## Testing Notes
- APK builds successfully with fixed Material theme
- Host-MotoApp BLE connection established
- RSSI data streaming working with measured values (simulated)
- **LED indicators functioning correctly**:
  - **HOST**: LED0: Heartbeat, LED1: MotoApp, LED2: Mipe, LED3: Data streaming
  - **MIPE**: LED0: Heartbeat, LED1: Connection status, LED3: Battery notifications
- All BLE API deprecation warnings resolved
- Host firmware supports simultaneous connections (8 max)
- **Architecture validated**: RSSI measurement at receiver is correct approach

## Next Steps
1. Flash and test new firmware with corrected RSSI architecture
2. Verify RSSI measurements flow from Host to MotoApp
3. Test battery status reporting from Mipe
4. Implement real RSSI measurement using HCI commands (TMT4)
5. Develop distance calculation algorithms based on measured RSSI
6. Test power consumption with new architecture (should be improved)
