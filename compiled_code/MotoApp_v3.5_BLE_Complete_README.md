# MotoApp v3.5 - Complete BLE Implementation

## Version Information
- **Version:** 3.5
- **Date:** August 23, 2025
- **File:** MotoApp_TMT1_v3.5_BLE_Complete.apk
- **Size:** 15.7 MB

## Overview
MotoApp v3.5 is the complete BLE-enabled version with all permission fixes applied. The app can now successfully:
- Scan for and detect BLE devices
- Connect to the Host device (MIPE_HOST_A1B2)
- Stream RSSI data in real-time
- Calculate and display distance measurements
- Switch between Real BLE and Simulation modes

## Key Features Implemented

### 1. Fixed BLE Permissions (from v3.4)
- **Location permissions** now properly requested for all Android versions
- Android 12+ gets location permission along with Bluetooth permissions
- Permission status display in debug screen
- Clear error messages when permissions are missing

### 2. Main App Functionality
- **Real BLE Mode**: Connects to actual Host device via Bluetooth
- **Simulation Mode**: Uses mock data for testing without hardware
- **Mode Toggle**: Easy switching between Real BLE and Simulation
- **Connection Management**: Connect/disconnect with visual feedback
- **Data Streaming**: Start/stop RSSI data stream from Host
- **RSSI Graph**: Real-time visualization of signal strength
- **Distance Calculation**: RSSI-based distance estimation

### 3. UI Components

#### Connection Section
- Shows current connection status (Disconnected/Scanning/Connected)
- Connect/Disconnect button with appropriate states
- Displays connected device name when connected

#### Data Stream Control
- Start/Stop data streaming button
- Only enabled when connected to Host

#### RSSI Histogram
- Real-time graph showing RSSI values over time
- Visual representation of signal strength variations

#### Status Display Panel
- Host connection status
- Data stream status
- Update rate and packet count
- Host device information (name, battery, signal strength)
- Connection duration timer

#### Distance Calculation Section
- Large distance display in meters
- Algorithm information
- Confidence level
- Distance statistics (average, min, max)
- Sample count

## Technical Implementation

### BLE Architecture
```
MotoApp (Android)
    ├── BleScanner - Finds Host device
    ├── BleManager - Manages GATT connection
    └── MotoAppBleViewModel - Coordinates BLE operations

Host Device (nRF54L15DK)
    ├── Advertises as "MIPE_HOST_A1B2"
    ├── Service UUID: 12345678-1234-5678-1234-56789abcdef0
    └── Sends RSSI data packets
```

### Permission Requirements
- **Android 6-11**: ACCESS_FINE_LOCATION
- **Android 12+**: BLUETOOTH_SCAN, BLUETOOTH_CONNECT, ACCESS_FINE_LOCATION

### Data Flow
1. User grants permissions → BLE scanner activates
2. Scanner finds "MIPE_HOST_A1B2" → Connects via GATT
3. User starts data stream → Host sends RSSI packets
4. App receives RSSI → Calculates distance → Updates UI

## Testing Instructions

### Initial Setup
1. Install MotoApp_TMT1_v3.5_BLE_Complete.apk
2. Grant all requested permissions:
   - Bluetooth permissions (Android 12+)
   - Location permission (all versions)
3. Enable Bluetooth on your device
4. Ensure Host device is powered and advertising

### Connection Test
1. Launch MotoApp - opens to main screen
2. Verify mode shows "Real BLE" (toggle if needed)
3. Press "CONNECT TO HOST" button
4. Wait for scanning (button shows "SCANNING...")
5. When connected, button changes to "DISCONNECT HOST"
6. Device name "MIPE_HOST_A1B2" appears below button

### Data Streaming Test
1. After successful connection
2. Press "START DATA STREAM" button
3. Watch RSSI graph update in real-time
4. Distance display shows calculated distance
5. Status panel shows packet count increasing
6. Press "STOP DATA STREAM" to pause

### Debug Mode
To access the debug scanner (if needed):
1. Modify MainActivity.kt to use DebugScreen instead of MainScreen
2. Rebuild the app
3. Debug screen shows:
   - Permission status for all required permissions
   - All BLE devices in range
   - Service UUID detection
   - Enhanced logging

## Troubleshooting

### Cannot Find Any BLE Devices
1. Check Permission Status Card in debug mode
2. Ensure all permissions show ✅
3. Verify Bluetooth is enabled
4. Check location services are enabled
5. Try force-closing and reopening the app

### Cannot Find Host Device
1. Verify Host is advertising (check with nRF Connect)
2. Ensure Host firmware is correct version
3. Check Host LEDs indicate advertising state
4. Try power cycling the Host device

### Connection Drops Frequently
1. Keep devices within 10 meters
2. Minimize obstacles between devices
3. Check Host firmware version (use stable streaming version)
4. Monitor RSSI values - should be > -80 dBm

### No Data Streaming
1. Ensure connection is established first
2. Press "START DATA STREAM" after connecting
3. Check Host LED indicates data streaming
4. Monitor packet count in status panel

## Known Issues & Limitations

1. **Host Firmware Compatibility**: Some Host firmware versions have issues with dual-role BLE (scanning for MIPE while advertising to MotoApp)
2. **Distance Accuracy**: RSSI-based distance calculation has ±2m accuracy
3. **Battery Impact**: Continuous BLE scanning and streaming impacts battery life
4. **Background Operation**: App must remain in foreground for BLE to work

## Version History

- **v3.0**: Initial BLE implementation (had UUID filtering issues)
- **v3.1**: Fixed BLE manager deprecation warnings
- **v3.2**: Added debug scanner screen
- **v3.3**: Enhanced debug scanner with better logging
- **v3.4**: Fixed critical permission issues for BLE scanning
- **v3.5**: Complete BLE implementation with main app functionality

## Next Development Steps

1. **Background Service**: Implement BLE operations in a foreground service
2. **Auto-reconnection**: Automatically reconnect when Host comes in range
3. **Multiple Device Support**: Connect to multiple MIPE devices
4. **Data Logging**: Save RSSI/distance data to file
5. **Calibration**: Add distance calibration feature
6. **Battery Optimization**: Implement adaptive scanning rates

## Technical Notes

- Built with Kotlin and Jetpack Compose
- Uses Nordic Android BLE Library v2.7.0
- Minimum SDK: API 21 (Android 5.0)
- Target SDK: API 33 (Android 13)
- Gradle 9.0 with Kotlin 1.9.0
