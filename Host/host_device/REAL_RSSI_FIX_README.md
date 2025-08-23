# Host Device Real RSSI Measurement Fix

## Issue Description
The Host firmware was not reading real RSSI values from the Mipe device connection. Instead, it was generating simulated RSSI values in the `rssi_work_handler` function in `ble_central.c`.

## Root Cause
The original implementation used simulated RSSI values as a "temporary workaround for TMT3 testing":
```c
/* For now, use simulated RSSI until we can access real RSSI */
/* Real RSSI would require BT_HCI_OP_READ_RSSI HCI command */
static int8_t simulated_rssi = -55;
static int8_t rssi_variation = 0;
```

## Solution
Modified the `rssi_work_handler` function to read real RSSI from the BLE connection using Zephyr's connection info API:

```c
/* Read real RSSI from the connection */
struct bt_conn_info info;
int err = bt_conn_get_info(mipe_conn, &info);
if (err) {
    LOG_ERR("Failed to get connection info: %d", err);
    return;
}

/* Get RSSI value from connection info */
int8_t rssi = info.le.rssi;
```

## Key Changes
1. Replaced simulated RSSI generation with `bt_conn_get_info()` call
2. Extract RSSI from `info.le.rssi` field
3. Added validation to check if RSSI is available (0x7F indicates unavailable)
4. Maintained the same callback mechanism to forward RSSI to MotoApp

## Data Flow
1. **Mipe Device** advertises and connects to Host
2. **Host Device** measures RSSI from the BLE connection every 1 second
3. **Host Device** forwards real RSSI values to MotoApp via BLE notifications
4. **MotoApp** displays the real RSSI data in the UI and calculates distance

## Building the Firmware
```bash
cd Host/host_device
./build_real_rssi.ps1
```

## Testing
1. Flash the new firmware to the Host device
2. Ensure Mipe device is powered on and advertising
3. Connect MotoApp to Host device
4. Enable "Real BLE" mode in MotoApp
5. Start data streaming
6. Verify that RSSI values change based on actual distance between Host and Mipe

## Expected Behavior
- RSSI values should typically range from -40 dBm (very close) to -90 dBm (far away)
- Values should change smoothly as you move the Mipe device
- No more fixed pattern of simulated data (-55 dBm ± 10)

## Notes
- The RSSI measurement rate is limited to 1Hz for stability
- There's a 2-second rate limit between notifications to MotoApp to prevent connection issues
- The fix maintains all existing stability improvements from previous versions
