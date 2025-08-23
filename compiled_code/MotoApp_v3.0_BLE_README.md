# MotoApp v3.0 BLE - Full BLE Implementation

## Version Information
- **File**: MotoApp_TMT1_v3.0_BLE.apk
- **Build Date**: August 22, 2025
- **Size**: 15.6 MB

## Key Features
This version includes full BLE (Bluetooth Low Energy) connectivity support, moving beyond the simulation-only capabilities of v2.0.

### BLE Capabilities
- **Real BLE Scanning**: Scans for Host device advertising as "MIPE_HOST_A1B2"
- **GATT Service Support**: Connects to service UUID `12345678-1234-5678-1234-56789abcdef0`
- **Data Characteristics**:
  - RSSI Data: `12345678-1234-5678-1234-56789abcdef1`
  - Distance Data: `12345678-1234-5678-1234-56789abcdef2`
- **Dual Mode Operation**: Toggle between Real BLE and Simulation modes

### Technical Implementation
- **BLE Manager**: Full Nordic BLE library integration for stable connections
- **BLE Scanner**: 10-second scan timeout with automatic device filtering
- **ViewModel**: Enhanced MotoAppBleViewModel supporting both modes
- **Permissions**: Complete Android 12+ BLE permissions handling

### UI Features
- Real-time RSSI graph visualization
- Distance calculation using: Distance = 10^((Tx_Power - RSSI) / (10 * N))
- Mode toggle chip for switching between BLE and Simulation
- Connection status indicators
- Live data streaming display

## Requirements
- Android 6.0 (API 23) or higher
- Bluetooth Low Energy support
- Location permissions for BLE scanning (Android requirement)

## Testing Instructions
1. Install the APK on an Android device
2. Grant Bluetooth and Location permissions when prompted
3. Ensure Host firmware is running and advertising as "MIPE_HOST_A1B2"
4. Toggle to "Real BLE" mode using the chip selector
5. The app will automatically scan and connect to the Host device
6. Monitor RSSI values and calculated distances in real-time

## Version History
- **v1.0**: Initial release with basic UI
- **v2.0**: Added simulation mode with mock data
- **v3.0**: Full BLE implementation with real device connectivity
- **v3.1**: Fixed BLE scanning - removed UUID filter to detect all devices by name

## Troubleshooting

### Device Not Found During Scanning
- **v3.1 Fix Applied**: The scanner now looks for devices by name only, not by service UUID
- The app will log all discovered BLE devices to help with debugging
- Ensure the Host device is advertising with the exact name "MIPE_HOST_A1B2"
- Check Android logcat for detailed scan results: `adb logcat | grep -E "BleScanner|MotoAppBleViewModel"`

## Known Compatible Firmware
- host_device_stable_streaming_20250821_rev012.hex
- host_device_ble_stability_fixes_20250821_rev011.hex
- Any Host firmware advertising as "MIPE_HOST_A1B2" with matching GATT services
