# Host On-Demand Scanning Implementation
**Date:** September 2, 2025, 21:16
**Version:** Host_250902_2116_ON_DEMAND_SCANNING

## Summary
Implemented on-demand BLE scanning to optimize power consumption. The Host now only scans for Mipe when the app explicitly requests streaming, rather than constantly scanning from boot.

## Changes Made

### 1. Host/host_device/src/main.c
- **Removed:** Automatic `ble_central_start_scan()` call on boot
- **Added:** Log message indicating Host is waiting for app command
- **Result:** Host boots idle, waiting for START_STREAM command

### 2. Host/host_device/src/ble/ble_peripheral.c
- **Added:** Include `ble_central.h` header
- **Modified CMD_START_STREAM handler:**
  - Now calls `ble_central_start_scan()` when streaming starts
  - Sends log notification to app about scan status
- **Modified CMD_STOP_STREAM handler:**
  - Now calls `ble_central_stop_scan()` when streaming stops
  - Sends log notification to app about scan status

## Power Optimization Benefits
1. **Reduced idle power consumption** - No scanning when app not connected
2. **On-demand operation** - Scanning only active during data streaming
3. **Clean start/stop** - Proper scan lifecycle management

## Operation Flow
1. Host boots and initializes BLE
2. Host starts advertising to app (MIPE_HOST_A1B2)
3. Host waits idle - NO scanning for Mipe
4. App connects and sends START_STREAM command
5. Host starts scanning for Mipe ("SinglePing Mipe")
6. Host streams RSSI/battery data while scanning
7. App sends STOP_STREAM command
8. Host stops scanning for Mipe
9. Returns to idle state

## Log Messages
- Boot: `"Waiting for app to start streaming before scanning for Mipe"`
- Start: `"Started scanning for Mipe device"`
- Stop: `"Stopped scanning for Mipe device"`

## Testing Notes
- Verify Host doesn't scan on boot (check power consumption)
- Confirm scanning starts with START_STREAM command
- Confirm scanning stops with STOP_STREAM command
- Monitor battery life improvement in idle state

## Build Command
```powershell
cd c:/Development/SinglePingProject
.\BAT files\build-flash-host.bat
```

## Related Files
- Host/host_device/src/main.c
- Host/host_device/src/ble/ble_peripheral.c
- Host/host_device/src/ble/ble_central.h
