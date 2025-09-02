# Host Build 250902_2014 - Battery Bundle Fix

## Build Information
- **Date**: September 2, 2025, 20:14
- **Version**: Host_250902_2014.hex
- **Status**: BUILD SUCCESSFUL & FLASHED

## Issues Fixed

### 1. Battery Bundle Logging
**Changed**: Now logs EVERY bundle sent to the app (not just every 5 seconds)
- **File**: `Host/host_device/src/ble/ble_peripheral.c`
- **Before**: Logged battery bundle only every 5 seconds
- **After**: Logs every single bundle sent with format:
  ```
  Battery Bundle: Host=7654 mV, Mipe=3322 mV, RSSI=-38 dBm
  ```

### 2. Removed Confusing Log Message
**Removed**: The "4-byte format" log message that was incorrect
- The system IS sending 5-byte packets (host_battery + mipe_battery + rssi)
- Removed the misleading log that said "4-byte format"

### 3. Fixed Mipe Battery Logging
**Fixed**: Corrected LOG_INF format string in ble_central.c
- **File**: `Host/host_device/src/ble/ble_central.c`
- **Before**: Had extra parameters that didn't match format string
- **After**: Clean logging: `Mipe battery: 3322 mV`

## Current Packet Format (5 bytes)
- Bytes 0-1: Host battery voltage (uint16_t, little-endian, mV)
- Bytes 2-3: Mipe battery voltage (uint16_t, little-endian, mV)
- Byte 4: RSSI value (int8_t)

## Expected Behavior
1. Host device scans for "SinglePing Mipe" beacons
2. Reads RSSI and battery data from Mipe advertising packets
3. When streaming is active, sends 5-byte bundles to MotoApp
4. Each bundle contains:
   - Host battery: 7654 mV (constant test value)
   - Mipe battery: From advertising data (e.g., 3322 mV)
   - RSSI: Real-time value from beacon

## Testing Notes
- Host battery is using constant 7654 mV for testing
- Mipe battery should show actual value from advertising (e.g., 3322 mV)
- Every packet sent to app will be logged to UART
- Monitor serial output at 115200 baud to verify

## Memory Usage
```
FLASH: 312420 B / 1428 KB (21.37%)
RAM:   81980 B / 188 KB (42.58%)
```

## Files Modified
1. `Host/host_device/src/ble/ble_peripheral.c` - Battery logging changes
2. `Host/host_device/src/ble/ble_central.c` - Fixed LOG_INF format

## Related Builds
- Android App: MotoApp_250902_1955.apk (with type fix)
- This Host build properly sends battery data to the app
