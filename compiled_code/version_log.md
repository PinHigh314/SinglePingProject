# Firmware Version Log

## Host Device Firmware

### host_device_test_rev005_arch_v4_20250823_rev017.hex
- **Date:** 2025-08-23 10:48
- **Description:** Test firmware v4 mimicking rev005 architecture
- **Changes:**
  - Uses same data flow as working rev005: BLE Central → mipe_rssi_callback → MotoApp
  - BLE Central generates fixed RSSI = -55 dBm via timer/work queue
  - Callback in main.c forwards data to MotoApp (no direct timer sending)
  - Modified ble_central.c to generate simulated RSSI without real Mipe connection
  - Simulates Mipe connection when MotoApp connects to trigger RSSI generation
- **Memory:** 311KB FLASH (21.29%), 81KB RAM (42.31%)
- **Notes:** This architecture matches rev005 which was confirmed working. Should resolve data flow issues seen in v1-v3.

### host_device_test_diagnostic_v3_20250823_rev016.hex
- **Date:** 2025-08-23 10:24
- **Description:** Diagnostic firmware v3 with enhanced debugging for data flow issues
- **Changes:**
  - Added detailed logging of every notification attempt
  - System status reports every 5 seconds
  - Warning messages if streaming active but no packets sent
  - Complete error code analysis for failed transmissions
  - Step-by-step logging through the notification process
  - All v2 fixes included (LED3 monitor, RSSI attribute fix)
- **Memory:** 320KB FLASH (21.90%), 81KB RAM (42.36%)
- **Notes:** Diagnostic build to identify why RSSI data isn't reaching MotoApp. Watch debug logs for detailed failure analysis.

### host_device_test_fixed_rssi_v2_20250823_rev015.hex
- **Date:** 2025-08-23 10:12
- **Description:** Test firmware v2 with fixes for LED3 turn-off and RSSI transmission
- **Changes:**
  - Fixed LED3 to properly turn OFF when streaming stops (monitor timer implementation)
  - Fixed RSSI characteristic attribute indexing for proper data transmission
  - Force-enables notifications when streaming starts
  - Sends fixed RSSI value of -55 dBm every 1 second
  - LED3 stays solid ON during data streaming (not flashing)
- **Memory:** 320KB FLASH (21.90%), 81KB RAM (42.36%)
- **Notes:** Fixes the two critical issues from rev014 - LED3 now turns off properly and RSSI data should transmit correctly

### host_device_test_fixed_rssi_20250823_rev014.hex
- **Date:** 2025-08-23 08:56
- **Description:** Test firmware with fixed RSSI = -55 dBm for debugging data transmission
- **Changes:**
  - Sends fixed RSSI value of -55 dBm every 1 second
  - LED3 stays solid ON during data streaming (not flashing)
  - Simplified data transmission for troubleshooting
  - Uses timer-based transmission instead of work queue
- **Memory:** 320KB FLASH (21.90%), 81KB RAM (42.36%)
- **Notes:** Test build to isolate data transmission issues from RSSI measurement

### host_device_real_rssi_measurement_20250823_rev013.hex
- **Date:** 2025-08-23 08:45
- **Description:** Prepared for real RSSI measurement implementation
- **Changes:**
  - Added TODO comment for HCI-based RSSI measurement
  - Identified Zephyr v3.1.0 limitation: bt_conn_le_info lacks rssi field
  - Requires BT_HCI_OP_READ_RSSI (0x1405) implementation
  - Currently still uses simulated RSSI with variation
- **Memory:** 320KB FLASH (21.90%), 81KB RAM (42.36%)
- **Notes:** Architecture ready for real RSSI, needs HCI implementation

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

### MotoApp_TMT1_v3.5_BLE_Complete.apk
- **Date:** 2025-08-22
- **Description:** Complete BLE implementation with permission fixes
- **Changes:**
  - Fixed Android 12+ location permission requirements
  - MainActivity now requests location permission for BLE scanning
  - Enhanced debug screen with detailed BLE device information
  - Shows all discovered devices, not just filtered ones
  - Complete error handling and status reporting
- **Size:** ~23 MB
- **Notes:** First version with working BLE device discovery on Android 12+

### MotoApp_TMT1_v3.4_Permission_Fix.apk
- **Date:** 2025-08-22
- **Description:** Fixed critical Android 12+ permission issue
- **Changes:**
  - Removed maxSdkVersion="30" restriction from ACCESS_FINE_LOCATION
  - Location permission now works on Android 12+ (API 31+)
  - Required for BLE scanning on modern Android devices
- **Size:** ~23 MB
- **Notes:** Addresses critical BLE scanning permission issue

### MotoApp_TMT1_v3.0_BLE_Integration.apk
- **Date:** 2025-08-22
- **Description:** Initial BLE integration with dual-mode support
- **Changes:**
  - Added complete BLE implementation using Nordic Android BLE Library
  - Dual-mode operation: Real BLE and Simulation modes
  - BLE scanner to find "MIPE_HOST_A1B2" device
  - GATT client for RSSI data streaming
  - Debug screen for BLE troubleshooting
- **Size:** ~23 MB
- **Notes:** Foundation for real BLE connectivity

### MotoApp_TMT1_v1.0_FW1.0.apk
- **Date:** 2025-08-22
- **Description:** Initial MotoApp with simulation mode
- **Changes:**
  - Complete UI implementation with Jetpack Compose
  - MVVM architecture with StateFlow
  - Simulation mode with mock RSSI data
  - RSSI graph visualization
  - Distance calculation from RSSI
- **Size:** ~22 MB
- **Notes:** Simulation-only version for UI testing

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
23/08/2025 14.33.57,15 - Built: host_device_build_082025_rev008.hex 
2025-08-23 14:43 - Built: mipe_device_tmt3_build_20250823_rev022.hex
23/08/2025 16.55.59,00 - Built: host_device_build_082025_rev008.hex 
23/08/2025 17.56.40,48 - Built: host_device_build_082025_rev008.hex 
23/08/2025 17.58.37,82 - Built: host_device_build_082025_rev008.hex 
23/08/2025 17.58.52,10 - Built: host_device_build_082025_rev008.hex 
23/08/2025 17.59.02,17 - Built: host_device_build_082025_rev008.hex 
23/08/2025 18.07.58,22 - Built: host_device_build_082025_rev008.hex 
23/08/2025 18.08.14,08 - Built: host_device_build_082025_rev008.hex 
23/08/2025 18.17.07,28 - Built: host_device_build_082025_rev008.hex 
23/08/2025 18.19.13,32 - Built: host_device_build_082025_rev008.hex 
23/08/2025 18.22.03,46 - Built: host_device_build_082025_rev001.hex 
23/08/2025 18.22.35,01 - Built: host_device_build_082025_rev000.hex 
23/08/2025 18.24.16,45 - Built: host_device_build_082025_rev001.hex 
23/08/2025 18.26.40,19 - Built: host_device_build_082025_rev001.hex 
23/08/2025 18.32.57,19 - Built: host_device_build_082025.hex 
23/08/2025 18.40.59,53 - Built: host_device_build_082025.hex 
23/08/2025 18.48.40,05 - Built: mipe_device_build_082025_rev008.hex 
23/08/2025 18.57.09,37 - Built: host_device_build_082025.hex 
23/08/2025 18.57.34,92 - Built: mipe_device_build_082025_rev008.hex 
23/08/2025 18.58.25,57 - Built: mipe_device_build_082025_rev008.hex 
23/08/2025 19.03.02,15 - Built: host_device_build_082025.hex 
23/08/2025 19.06.24,26 - Built: host_device_build_082025.hex 
23/08/2025 19.11.21,88 - Built: host_device_build_082025.hex 
23/08/2025 20.00.09,02 - Built: host_device_build_082025.hex 
23/08/2025 20.05.04,35 - Built: host_device_build_082025.hex 
23/08/2025 20.26.25,35 - Built: host_device_build_082025.hex 
23/08/2025 20.36.20,43 - Built: host_device_build_082025.hex 
23/08/2025 20.43.08,43 - Built: host_device_build_082025.hex 
23/08/2025 20.48.47,68 - Built: host_device_build_082025.hex 
23/08/2025 21.01.06,54 - Built: host_device_build_082025.hex 
23/08/2025 22.05.17,88 - Built: host_device_build_082025.hex 
23/08/2025 22.16.32,92 - Built: host_device_build_082025.hex 
23/08/2025 22.21.25,38 - Built: host_device_build_082025.hex 
24/08/2025  6.54.11,85 - Built: host_device_build_082025.hex 
24/08/2025  7.23.37,36 - Built: host_device_build_082025.hex 
24/08/2025  7.49.11,85 - Built: host_device_build_082025.hex 
24/08/2025  8.32.09,22 - Built: host_device_build_082025.hex 
24/08/2025  8.35.36,74 - Built: host_device_build_082025.hex 
24/08/2025  8.43.21,02 - Built: host_device_build_082025.hex 
24/08/2025 10.35.30,14 - Built: Mipe_250824_1035.hex 
24/08/2025 10.39.41,11 - Built: Host_250824_1039.hex 
24/08/2025 13.21.35,35 - Built: Host_250824_1321.hex 
24/08/2025 15.30.33,97 - Built: Host_250824_1530.hex 
24/08/2025 15.30.54,32 - Built: Host_250824_1530.hex 
24/08/2025 15.55.54,09 - Built: Host_250824_1555.hex 
24/08/2025 15.56.17,15 - Built: Host_250824_1556.hex 
24/08/2025 16.06.26,60 - Built: Host_250824_1606.hex 
24/08/2025 16.06.38,29 - Built: Host_250824_1606.hex 
24/08/2025 16.14.47,15 - Built: Host_250824_1614.hex 
24/08/2025 16.15.42,37 - Built: Host_250824_1615.hex 
24/08/2025 16.27.19,31 - Built: Host_250824_1627.hex 
24/08/2025 16.32.49,59 - Built: Host_250824_1632.hex 
24/08/2025 16.36.21,36 - Built: Host_250824_1636.hex 
24/08/2025 16.46.28,45 - Built: Host_250824_1646.hex 
24/08/2025 17.03.52,44 - Built: Host_250824_1703.hex 
24/08/2025 17.37.40,06 - Built: Host_250824_1737.hex 
24/08/2025 17.44.07,72 - Built: Host_250824_1744.hex 
24/08/2025 19.31.51,74 - Built: Host_250824_1931.hex 
24/08/2025 19.34.25,20 - Built: Host_250824_1934.hex 
24/08/2025 19.46.47,51 - Built: Host_250824_1946.hex 
31/08/2025 11.31.13,04 - Built: Host_250831_1131.hex 
01/09/2025 16.35.42,05 - Built: Host_250901_1635.hex 
01/09/2025 17.07.51,35 - Built: Host_250901_1707.hex 
01/09/2025 17.14.59,39 - Built: Host_250901_1714.hex 
01/09/2025 17.25.00,21 - Built: Host_250901_1725.hex 
01/09/2025 17.25.39,50 - Built: Host_250901_1725.hex 
01/09/2025 17.32.23,47 - Built: Host_250901_1732.hex 
01/09/2025 17.40.51,73 - Built: Host_250901_1740.hex 
01/09/2025 17.58.25,60 - Built: Host_250901_1758.hex 
01/09/2025 18.48.35,29 - Built: Host_250901_1848.hex 
01/09/2025 18.49.22,11 - Built: Host_250901_1849.hex 
02/09/2025  7.51.26,69 - Built: Mipe_250902_0751.hex 
02/09/2025 10.03.48,77 - Built: Mipe_250902_1003.hex 
02/09/2025 10.07.20,22 - Built: Mipe_250902_1007.hex 
02/09/2025 10.11.15,31 - Built: Mipe_250902_1011.hex 
02/09/2025 10.14.19,70 - Built: Mipe_250902_1014.hex 
02/09/2025 10.18.47,02 - Built: Mipe_250902_1018.hex 
02/09/2025 10.26.35,54 - Built: Mipe_250902_1026.hex 
02/09/2025 10.32.33,05 - Built: Mipe_250902_1032.hex 
02/09/2025 10.34.24,67 - Built: Mipe_250902_1034.hex 
02/09/2025 10.58.50,43 - Built: Mipe_250902_1058.hex 
02/09/2025 11.00.06,86 - Built: Mipe_250902_1100.hex 
02/09/2025 11.15.17,20 - Built: Mipe_250902_1115.hex 
02/09/2025 11.18.41,65 - Built: Mipe_250902_1118.hex 
02/09/2025 11.19.26,37 - Built: Host_250902_1119.hex 
02/09/2025 11.21.00,44 - Built: Host_250902_1121.hex 
02/09/2025 14.01.33,23 - Built: Host_250902_1401.hex 
02/09/2025 14.07.04,79 - Built: Host_250902_1407.hex 
02/09/2025 14.08.29,76 - Built: Host_250902_1408.hex 
02/09/2025 14.09.08,68 - Built: Mipe_250902_1409.hex 
02/09/2025 14.43.21,64 - Built: Host_250902_1443.hex 
02/09/2025 14.45.02,63 - Built: Mipe_250902_1445.hex 
02/09/2025 14.46.46,15 - Built: Mipe_250902_1446.hex 
02/09/2025 14.47.30,05 - Built: Mipe_250902_1447.hex 
02/09/2025 14.52.27,60 - Built: Mipe_250902_1452.hex 
02/09/2025 14.53.02,64 - Built: Mipe_250902_1453.hex 
02/09/2025 14.53.05,89 - Built: Host_250902_1453.hex 
02/09/2025 14.53.43,74 - Built: Host_250902_1453.hex 
02/09/2025 14.55.36,70 - Built: Host_250902_1455.hex 
02/09/2025 17.10.12,45 - Built: Mipe_250902_1710.hex 
02/09/2025 17.11.04,37 - Built: Mipe_250902_1711.hex 
02/09/2025 17.12.07,78 - Built: Host_250902_1712.hex 
02/09/2025 18.15.05,53 - Built: Host_250902_1815.hex 
02/09/2025 18.17.53,62 - Built: Host_250902_1817.hex 
02/09/2025 18.27.44,68 - Built: Host_250902_1827.hex 
02/09/2025 18.32.57,33 - Built: Host_250902_1832.hex 
02/09/2025 18.35.29,64 - Built: Host_250902_1835.hex 
02/09/2025 18.39.51,04 - Built: Host_250902_1839.hex 
02/09/2025 18.40.31,79 - Built: Host_250902_1840.hex 
02/09/2025 18.50.06,94 - Built: Host_250902_1850.hex 
02/09/2025 18.52.40,13 - Built: Host_250902_1852.hex 
02/09/2025 18.59.38,45 - Built: Host_250902_1859.hex 
02/09/2025 19.43.04,67 - Built: Host_250902_1943.hex 
02/09/2025 19.49.18,97 - Built: Host_250902_1949.hex 
02/09/2025 20.14.46,71 - Built: Host_250902_2014.hex 
02/09/2025 21.12.39,62 - Built: Host_250902_2112.hex 
02/09/2025 21.17.23,46 - Built: Host_250902_2117.hex 
02/09/2025 21.27.03,30 - Built: Host_250902_2127.hex 
02/09/2025 21.31.44,74 - Built: Host_250902_2131.hex 
02/09/2025 21.32.33,61 - Built: Host_250902_2132.hex 
02/09/2025 21.34.42,47 - Built: Host_250902_2134.hex 
02/09/2025 21.38.04,25 - Built: Host_250902_2138.hex 
02/09/2025 21.40.46,15 - Built: Host_250902_2140.hex 
02/09/2025 21.48.20,86 - Built: Host_250902_2148.hex 
02/09/2025 21.53.51,00 - Built: Host_250902_2153.hex 
02/09/2025 21.57.21,70 - Built: Host_250902_2157.hex 
02/09/2025 22.00.14,98 - Built: Host_250902_2200.hex 
02/09/2025 22.04.05,49 - Built: Host_250902_2204.hex 
03/09/2025 12.29.05,88 - Built: MotoApp_250903_1229.apk 
03/09/2025 12.31.06,99 - Built: MotoApp_250903_1231.apk 
03/09/2025 12.47.12,16 - Built: MotoApp_250903_1247.apk 
03/09/2025 12.52.19,14 - Built: MotoApp_250903_1252.apk 
03/09/2025 12.53.35,61 - Built: MotoApp_250903_1253.apk 
03/09/2025 12.54.55,23 - Built: MotoApp_250903_1254.apk 
03/09/2025 12.55.15,25 - Built: MotoApp_250903_1255.apk 
03/09/2025 12.56.35,60 - Built: MotoApp_250903_1256.apk 
03/09/2025 12.57.06,32 - Built: MotoApp_250903_1257.apk 
03/09/2025 13.50.58,80 - Built: MotoApp_250903_1350.apk 
03/09/2025 13.52.26,90 - Built: MotoApp_250903_1352.apk 
03/09/2025 13.55.44,06 - Built: MotoApp_250903_1355.apk 
03/09/2025 13.55.59,81 - Built: MotoApp_250903_1355.apk 
03/09/2025 13.57.47,80 - Built: MotoApp_250903_1357.apk 
03/09/2025 16.07.20,58 - Built: MotoApp_250903_1607.apk 
03/09/2025 16.49.27,72 - Built: MotoApp_250903_1649.apk 
03/09/2025 18.47.08,39 - Built: MotoApp_250903_1847.apk 
03/09/2025 19.03.21,83 - Built: MotoApp_250903_1903.apk 
03/09/2025 19.37.01,54 - Built: MotoApp_250903_1937.apk 
03/09/2025 19.50.06,15 - Built: MotoApp_250903_1950.apk 
03/09/2025 19.56.41,48 - Built: MotoApp_250903_1956.apk 
03/09/2025 20.28.34,01 - Built: MotoApp_250903_2028.apk 
03/09/2025 21.05.33,14 - Built: MotoApp_250903_2105.apk 
03/09/2025 21.17.44,83 - Built: MotoApp_250903_2117.apk 
03/09/2025 21.27.56,99 - Built: MotoApp_250903_2127.apk 
04/09/2025  5.54.55,16 - Built: MotoApp_250904_0554.apk 
04/09/2025  5.55.36,42 - Built: MotoApp_250904_0555.apk 
04/09/2025  6.08.32,07 - Built: MotoApp_250904_0608.apk 
04/09/2025  6.43.41,21 - Built: MotoApp_250904_0643.apk 
04/09/2025  6.45.39,40 - Built: MotoApp_250904_0645.apk 
04/09/2025  9.30.21,67 - Built: MotoApp_250904_0930.apk 
04/09/2025  9.41.31,92 - Built: MotoApp_250904_0941.apk 
04/09/2025 10.04.09,61 - Built: MotoApp_250904_1004.apk 
04/09/2025 10.09.21,16 - Built: MotoApp_250904_1009.apk 
04/09/2025 10.18.54,74 - Built: MotoApp_250904_1018.apk 
04/09/2025 10.30.52,03 - Built: MotoApp_250904_1030.apk 
04/09/2025 10.49.11,30 - Built: MotoApp_250904_1049.apk 
04/09/2025 10.51.41,40 - Built: MotoApp_250904_1051.apk 
04/09/2025 12.16.58,34 - Built: MotoApp_250904_1216.apk 
04/09/2025 13.05.55,52 - Built: MotoApp_250904_1305.apk 
04/09/2025 13.37.59,93 - Built: MotoApp_250904_1337.apk 
04/09/2025 15.56.43,05 - Built: MotoApp_250904_1556.apk 
04/09/2025 17.22.10,55 - Built: MotoApp_250904_1722.apk 
04/09/2025 17.34.27,88 - Built: MotoApp_250904_1734.apk 
