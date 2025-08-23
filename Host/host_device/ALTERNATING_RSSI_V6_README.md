# Host Firmware v6 - Alternating RSSI Test Mode

## Overview
This test firmware alternates between sending fixed reference RSSI (-55 dBm) and real RSSI readings from the MotoApp connection. This provides both a known reference value to confirm data flow and dynamic values to test the system.

## Features
- **Alternating RSSI transmission:**
  - Fixed reference: -55 dBm (LED3 flashes)
  - Real RSSI: From MotoApp connection (LED2 flashes)
- **LED indicators:**
  - LED1: Solid when MotoApp connected
  - LED2: Flashes when transmitting real RSSI
  - LED3: Flashes when transmitting fixed -55 reference
- **Proper command-based streaming control**

## Files
- `src/main_test_fixed_rssi_v6.c` - Main firmware with alternating logic
- `src/ble/ble_central_test.c` - Test BLE central (required)

## Manual Build Instructions

### Option 1: Using nRF Connect for Desktop
1. Open nRF Connect for Desktop
2. Launch the Toolchain Manager
3. Open a terminal for nRF Connect SDK v3.1.0
4. Navigate to: `cd C:\Development\SinglePingProject\Host\host_device`
5. Copy v6 files:
   ```
   copy src\main_test_fixed_rssi_v6.c src\main.c
   copy src\ble\ble_central_test.c src\ble\ble_central.c
   ```
6. Build:
   ```
   west build -b nrf54l15dk/nrf54l15/cpuapp --pristine
   ```
7. Output will be in: `build\zephyr\zephyr.hex`

### Option 2: Using PowerShell with proper environment
```powershell
# Open PowerShell as Administrator
# Set up environment
$env:ZEPHYR_BASE = "C:\ncs\v3.1.0\zephyr"
$env:PATH = "C:\ncs\toolchains\b8b84efebd\opt\bin;$env:PATH"

# Navigate to project
cd C:\Development\SinglePingProject\Host\host_device

# Copy v6 files
Copy-Item "src\main_test_fixed_rssi_v6.c" "src\main.c" -Force
Copy-Item "src\ble\ble_central_test.c" "src\ble\ble_central.c" -Force

# Build
west build -b nrf54l15dk/nrf54l15/cpuapp --pristine
```

## Expected Behavior
1. Connect MotoApp → LED1 turns ON (solid)
2. Press "Start Streaming" in MotoApp
3. Data alternates between:
   - Fixed -55 dBm (LED3 flash 100ms)
   - Real RSSI value (LED2 flash 100ms)
4. MotoApp graph shows alternating values
5. Press "Stop Streaming" → LEDs 2 & 3 turn OFF

## Implementation Details
```c
/* Alternating logic in mipe_rssi_callback */
if (use_fixed_rssi) {
    rssi_to_send = -55;  // Fixed reference
    gpio_pin_set_dt(&led3, 1);  // Flash LED3
} else {
    rssi_to_send = /* real RSSI from connection */;
    gpio_pin_set_dt(&led2, 1);  // Flash LED2
}
use_fixed_rssi = !use_fixed_rssi;  // Toggle for next time
```

## Troubleshooting
- If build fails with Python errors, use nRF Connect for Desktop terminal
- Ensure both v6 files are copied before building
- Check that no other build processes are running
- Clean build directory if needed: `rm -rf build`

## Version Info
- Version: v6
- Revision: rev019
- Date: 2025-08-23
- Type: Test firmware with alternating RSSI
