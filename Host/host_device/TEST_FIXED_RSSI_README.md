# Host Device Test Firmware - Fixed RSSI Transmission

## Purpose
This test firmware was created to debug data transmission issues between Host and MotoApp. It sends a fixed RSSI value of -55 dBm to verify the data flow path.

## Test Firmware Features
1. **Fixed RSSI Value**: Sends constant -55 dBm (no variations)
2. **LED3 Behavior**: Solid ON during streaming (not toggling)
3. **No Mipe Required**: Works without Mipe device connection
4. **Transmission Rate**: Sends data every 1 second when streaming

## Files Created
- `src/main_test_fixed_rssi.c` - Test version of main.c
- `build_test_fixed_rssi.ps1` - Build script for test firmware
- `compiled_code/host_device_test_fixed_rssi_20250823_rev014.hex` - Built firmware

## How to Use
1. Flash the test firmware:
   ```
   nrfjprog --program compiled_code/host_device_test_fixed_rssi_20250823_rev014.hex --chiperase --verify -r
   ```

2. Connect MotoApp to Host device

3. Start data streaming in MotoApp

4. Verify:
   - LED3 turns solid ON when streaming starts
   - MotoApp receives constant -55 dBm RSSI values
   - Data arrives every 1 second

## Expected Behavior
- **LED0**: Heartbeat (blinking)
- **LED1**: Solid ON when MotoApp connected
- **LED3**: Solid ON when streaming data
- **Console**: Should show "Sent fixed RSSI: -55 dBm (packet X)" messages

## Troubleshooting
If data is not received in MotoApp:
1. Check LED3 - should be solid ON during streaming
2. Check console logs for "Sent fixed RSSI" messages
3. Verify BLE connection is stable (LED1 should be ON)
4. Check MotoApp is in Real BLE mode (not simulation)

## Reverting to Normal Firmware
To go back to the regular firmware with RSSI measurement:
```
cd Host/host_device
./build_real_rssi.ps1
```

## Key Differences from Normal Firmware
- Bypasses Mipe connection requirement
- Sends fixed data instead of measured/simulated RSSI
- LED3 stays solid instead of toggling
- Simplified data flow for debugging
