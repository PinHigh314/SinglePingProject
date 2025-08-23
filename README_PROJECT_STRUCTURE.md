# SinglePing Project Structure

## Overview
This project implements a BLE-based distance measurement system with three main components:

## Directory Structure

### `/Host/`
Contains the Host device firmware that runs on nRF54L15DK
- **`host_device/`** - Main Host firmware with BLE peripheral for MotoApp connection
  - Advertises as: "MIPE_HOST_A1B2"
  - Service UUID: `12345678-1234-5678-1234-56789abcdef0`
  - Handles dual-role BLE (peripheral for MotoApp, central for Mipe)
- **`tmt1_mock_data_host/`** - Simple LED blinker test (no BLE)

### `/Mipe/`
Contains the Mipe device firmware for distance measurement
- **`mipe_device/`** - Mipe device firmware with BLE peripheral
  - Advertises for Host device connection
  - Provides RSSI data for distance calculation

### `/MotoApp/`
Android application for monitoring and control
- Built with Kotlin and Jetpack Compose
- Connects to Host device via BLE
- Displays real-time RSSI and distance data
- Latest version: v3.1 with BLE fixes

### `/compiled_code/`
Pre-built binaries ready for deployment
- **Host firmware:** `host_device_stable_streaming_20250821_rev012.hex`
- **Mipe firmware:** Various versions for testing
- **MotoApp APKs:** 
  - `MotoApp_TMT1_v3.1_BLE_Fixed.apk` - Latest with scanning fixes
  - `MotoApp_TMT1_v3.0_BLE.apk` - Initial BLE implementation
  - `MotoApp_TMT1_v2.0.apk` - Simulation only

## System Architecture
```
MotoApp (Android) ←→ BLE ←→ Host (nRF54L15DK) ←→ BLE ←→ Mipe Device
```

## Quick Start

### 1. Flash Host Device
```bash
# Use J-Link or nRF Connect to flash:
compiled_code/host_device_stable_streaming_20250821_rev012.hex
```

### 2. Flash Mipe Device (if available)
```bash
# Use appropriate Mipe firmware from compiled_code/
```

### 3. Install MotoApp
```bash
# Install on Android device:
adb install compiled_code/MotoApp_TMT1_v3.1_BLE_Fixed.apk
```

### 4. Connect and Test
1. Ensure Host device is powered and advertising
2. Open MotoApp and grant BLE permissions
3. Toggle to "Real BLE" mode
4. Tap "Connect" to scan for Host device
5. Monitor RSSI and distance data

## Troubleshooting

### BLE Connection Issues
- Verify Host is advertising as "MIPE_HOST_A1B2" using nRF Connect app
- Check Android logs: `adb logcat | grep -E "BleScanner|MotoAppBleViewModel"`
- Ensure Bluetooth is enabled and permissions granted
- Try simulation mode first to verify app functionality

## Development Status
- **TMT1**: ✅ UI Foundation complete
- **TMT2**: ✅ Host-MotoApp BLE integration complete
- **TMT3**: 🔄 Mipe device integration in progress
- **TMT4**: ⏳ Real distance calculation pending
- **TMT5**: ⏳ RF optimization pending
- **TMT6**: ⏳ Power optimization pending
