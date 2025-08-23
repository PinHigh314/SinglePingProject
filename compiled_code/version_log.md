# Firmware Version Log

## Host Device Firmware

### host_device_stable_streaming_20250821_rev012.hex
- **Date:** 2025-08-21 13:58
- **Description:** Complete stability fix for dual-connection data streaming
- **Changes:**
  - Reduced RSSI measurement frequency from 10Hz to 1Hz for stability
  - Added 2-second rate limiting between RSSI notifications to MotoApp  
  - Added 50ms delay between BLE operations to ensure stack stability
  - Enhanced connection validation before sending notifications
  - Better error handling for buffer full and connection loss scenarios
- **Memory:** 320KB FLASH (21.90%), 81KB RAM (42.36%)
- **Notes:** Prioritizes reliability over speed - prevents connection breaking during streaming

### host_device_ble_stability_fixes_20250821_rev011.hex
- **Date:** 2025-08-21 13:42
- **Description:** Critical BLE stability fixes for MotoApp reconnection and data streaming
- **Changes:**
  - Fix 1: Force enable RSSI notifications when starting data stream (bypasses CCC callback issues)
  - Fix 2: Robust advertising restart with 5 retries and progressive backoff for MotoApp reconnection
  - Fix 3: Enhanced MIPE disconnection detection to ensure LED status updates properly
- **Memory:** 319KB FLASH (21.87%), 81KB RAM (42.36%)
- **Notes:** Addresses all reported BLE stability issues - data streaming, reconnection, and LED status

### host_device_dual_connection_20250821_rev010.hex
- **Date:** 2025-08-21 13:19
- **Description:** Dual-role BLE with connection requirements for data flow
- **Changes:**
  - Data streaming requires BOTH MotoApp and MIPE to be connected
  - Host measures RSSI from MIPE connection (simulated for TMT3)
  - Maintains sequential BLE operation as designed
- **Memory:** 319KB FLASH (21.85%), 81KB RAM (42.36%)
- **Notes:** Architecture enforces both connections for data flow as intended

### host_device_tmt3_scanning_fix_20250821_rev008.hex
- **Date:** 2025-08-21 12:50
- **Description:** Fixed Host-MotoApp BLE connection issue by delaying Mipe scanning
- **Changes:**
  - Removed automatic ble_central_start_scan() on initialization
  - Start Mipe scanning only after MotoApp connects
  - Stop Mipe scanning when MotoApp disconnects
  - Prevents BLE Central scanning from interfering with Peripheral advertising
- **Memory:** 324KB FLASH (22.18%), 80KB RAM (41.60%)
- **Notes:** Fixes critical issue where neither MotoApp nor BLE scanners could connect to Host

### host_device_tmt3_data_stream_20250821_rev007.hex
- **Date:** 2025-08-21 12:31
- **Description:** Data streaming investigation build
- **Changes:**
  - Verified data streaming works when both connections active
  - Data only flows when BOTH MotoApp and MIPE are connected
- **Memory:** 323KB FLASH (22.14%), 80KB RAM (41.56%)
- **Notes:** Data streaming functional but requires both connections

### host_device_tmt3_real_rssi_20250821_rev006.hex
- **Date:** 2025-08-21 12:18
- **Description:** Corrected RSSI architecture - Host measures from BLE signal
- **Changes:**
  - Removed mock RSSI generation from main.c
  - Implemented RSSI measurement from BLE connection (simulated for TMT3)
  - RSSI now measured at Host, not transmitted by MIPE
- **Memory:** 324KB FLASH (22.16%), 80KB RAM (41.60%)
- **Notes:** Architecturally correct RSSI measurement approach

### host_device_tmt3_gatt_discovery_20250821_rev005.hex
- **Date:** 2025-08-21 11:51
- **Description:** Added GATT discovery and auto-subscription
- **Changes:**
  - Added CONFIG_BT_GATT_CLIENT=y to prj.conf
  - Integrated GATT discovery directly into ble_central.c
  - Auto-subscribes to MIPE RSSI notifications on connection
- **Memory:** 323KB FLASH (22.14%), 80KB RAM (41.56%)
- **Notes:** Complete data flow path: MIPE → Host → MotoApp

### host_device_tmt3_dual_role_20250820_rev002.hex
- **Date:** 2025-08-20 18:35
- **Description:** TMT3 Dual-role BLE Implementation
- **Changes:**
  - Enabled simultaneous BLE connections (MotoApp + Mipe)
  - Host supports up to 8 concurrent BLE connections
  - Implemented dual-role: Peripheral (to MotoApp) + Central (to Mipe)
- **Memory:** 304KB FLASH (20.83%), 78KB RAM (40.66%)
- **Notes:** Complete data pipeline ready: Mipe → Host → MotoApp

### host_device_tmt3_dual_role_20250820_rev001.hex
- **Date:** 2025-08-20 18:30
- **Description:** Initial TMT3 dual-role BLE configuration
- **Changes:**
  - Added BLE Central module for Mipe connections
  - Configured dual-role operation
  - Increased max connections to 8
- **Memory:** 304KB FLASH (20.83%), 78KB RAM (40.66%)
- **Notes:** First build with dual-role capability

### host_device_tmt3_led_reconnect_fix_20250820_rev002.hex
- **Date:** 2025-08-20 19:01
- **Description:** Fixed LED behavior on reconnection
- **Changes:**
  - LED1 properly turns off on MotoApp disconnect
  - LED1 flashes during advertising after disconnect
  - LED1 goes solid when reconnected
- **Memory:** 306KB FLASH (20.97%), 78KB RAM (40.73%)
- **Notes:** Proper LED state management across connection cycles

### host_device_tmt3_reconnect_fix_20250820_rev001.hex
- **Date:** 2025-08-20 18:53
- **Description:** Fixed reconnection after MotoApp disconnect
- **Changes:**
  - Re-enable advertising after MotoApp disconnects
  - Clear connection reference on disconnect
  - Allow new connections after disconnect
- **Memory:** 305KB FLASH (20.91%), 78KB RAM (40.70%)
- **Notes:** MotoApp can now reconnect after disconnection

### host_device_tmt1_20250820_rev002.hex
- **Date:** 2025-08-20 17:58
- **Description:** Fixed LED assignments per Master Project Prompt
- **Changes:**
  - LED0: Heartbeat (1000ms intervals)
  - LED1: MotoApp connection status
  - LED2: Reserved for Mipe (TMT3+)
  - LED3: Data streaming activity
- **Memory:** 242KB FLASH (16.60%), 60KB RAM (31.25%)
- **Notes:** Correct LED mapping as specified

### host_device_tmt1_20250820_rev001.hex
- **Date:** 2025-08-20 07:45
- **Description:** Initial TMT1 build with heartbeat
- **Changes:**
  - Basic BLE Peripheral implementation
  - LED0 heartbeat at 1000ms intervals
  - GATT services for MotoApp
- **Memory:** 240KB FLASH (16.44%), 60KB RAM (31.25%)
- **Notes:** Foundation build for TMT testing

## Mipe Device Firmware

### mipe_device_led_fix_20250821_rev007.hex
- **Date:** 2025-08-21 13:16
- **Description:** Fixed MIPE LED mapping - LED1 now shows connection status
- **Changes:**
  - Fixed LED mapping in led_control.c
  - LED_ID_CONNECTION now correctly maps to LED1 (was LED2)
  - LED1 properly indicates when connected to Host
- **Memory:** 158KB FLASH (10.86%), 31KB RAM (16.37%)
- **Notes:** Resolves issue where LED2 was lighting instead of LED1

### mipe_device_tmt3_led_fix_20250821_rev006.hex
- **Date:** 2025-08-21 12:34
- **Description:** Fixed LED mapping for connection status
- **Changes:**
  - Fixed connection manager using wrong LED constant
  - Changed LED_ID_PAIRING to LED_ID_CONNECTION in all places
  - LED1 now correctly shows connection status (was LED2)
- **Memory:** 159KB FLASH (10.86%), 31KB RAM (16.37%)
- **Notes:** Correct LED behavior as per specification

### mipe_device_tmt3_real_rssi_20250821_rev005.hex
- **Date:** 2025-08-21 12:20
- **Description:** Removed fake RSSI characteristic - architecturally correct
- **Changes:**
  - Removed RSSI characteristic completely
  - MIPE now only provides battery status via GATT
  - RSSI is measured by Host from signal strength
- **Memory:** 159KB FLASH (10.86%), 31KB RAM (16.37%)
- **Notes:** Correct architecture - RSSI measured at receiver

### mipe_device_tmt3_led_fix_20250821_rev004.hex
- **Date:** 2025-08-21 11:36
- **Description:** Fixed LED1 to show connection status
- **Changes:**
  - Corrected LED assignments in connection manager
  - LED1 now properly indicates connection status
  - Fixed from incorrectly using LED2
- **Memory:** 159KB FLASH (10.89%), 31KB RAM (16.39%)
- **Notes:** Ready for deployment with correct LED indicators

### mipe_device_tmt3_ble_host_pattern_20250821_rev003.hex
- **Date:** 2025-08-21 02:00
- **Description:** Complete BLE service rewrite based on Host pattern
- **Changes:**
  - Rewritten BLE service following Host's proven ble_peripheral.c pattern
  - Device advertises as "MIPE" (not MIPE_001)
  - Using BT_LE_ADV_PARAM_INIT, BT_CONN_CB_DEFINE, BT_GATT_SERVICE_DEFINE
  - Fixed deprecated API warnings
- **Memory:** 159KB FLASH (10.89%), 31KB RAM (16.39%)
- **Notes:** Stable BLE implementation matching Host architecture

### Previous versions...
(Earlier versions omitted for brevity)

## MotoApp APK Versions

### MipeTest_v2.3_FW_TMT3_003.apk
- **Date:** 2025-08-21
- **Description:** Built with fixed BLE deprecation warnings
- **Changes:**
  - Updated to Android 13+ Bluetooth GATT APIs
  - Fixed all 12 deprecation warnings
  - Replaced deprecated writeCharacteristic() and writeDescriptor()
- **Size:** 22.4 MB
- **Notes:** Clean build with no Kotlin compilation warnings

### MipeTest_v1.1_FW_TMT1_002.apk
- **Date:** 2025-08-20
- **Description:** Fixed JDK configuration build
- **Changes:**
  - Resolved JDK mismatch issues
  - Fixed file lock problems
- **Size:** 22.3 MB
- **Notes:** Successfully built after JDK correction

### MipeTest_v1.0_FW_TMT1_001.apk
- **Date:** 2025-08-20
- **Description:** Initial TMT1 release
- **Changes:**
  - Complete UI implementation
  - Mock data for testing
  - Material theme fix applied
- **Size:** 22.3 MB
- **Notes:** Foundation build for TMT testing
