# Fixed V7 Firmware - Alternating RSSI with Correct LED Control

## Issue Resolution

The v6 firmware had critical bugs that prevented it from working:

### Problems in v6:
1. **Incorrect LED GPIO Mappings**: LED1 was mapped to led0 GPIO, LED2 to led1, etc.
2. **Missing Heartbeat**: No LED0 heartbeat made it impossible to see if firmware was running
3. **Wrong GPIO Check**: Used `device_is_ready()` instead of `gpio_is_ready_dt()`
4. **BLE Initialization Issues**: Due to the above problems, BLE never properly initialized

### Fixes in v7:
1. **Restored Correct LED Mappings** from v5:
   ```c
   static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
   static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
   static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
   static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);
   ```

2. **Added Heartbeat Timer** for LED0 (1Hz blink)
3. **Fixed GPIO Initialization** with proper `gpio_is_ready_dt()` checks
4. **Maintained Alternating RSSI Feature** from v6

## Firmware Location

The fixed firmware is ready:
```
compiled_code/host_device_test_fixed_alternating_rssi_v7_20250823_rev020.hex
```

## Testing Instructions

1. **Flash the v7 firmware**:
   ```bash
   nrfjprog --program compiled_code/host_device_test_fixed_alternating_rssi_v7_20250823_rev020.hex --chiperase --verify
   nrfjprog --reset
   ```

2. **Verify Basic Operation**:
   - LED0 should blink at 1Hz (heartbeat) - THIS IS THE KEY INDICATOR
   - Host should advertise as "SinglePing Host" in BLE scanners

3. **Test with MotoApp**:
   - Connect MotoApp to Host
   - LED1 should turn ON when connected
   - Press "Start Streaming"
   - LED3 should turn ON (streaming active)
   - Watch for alternating LED flashes:
     - LED2 flash = Real RSSI transmission
     - LED3 flash = Fixed -55 dBm transmission
   - Press "Stop Streaming"
   - LED3 should turn OFF

## LED Status Summary

| LED | Function | Expected Behavior |
|-----|----------|-------------------|
| LED0 | Heartbeat | Blinks continuously at 1Hz |
| LED1 | MotoApp Connected | ON when connected, OFF when disconnected |
| LED2 | Real RSSI TX | 200ms flash when transmitting real RSSI |
| LED3 | Fixed RSSI TX / Streaming | ON during streaming, 200ms flash for fixed RSSI |

## Build Information

Built using the universal build script:
```powershell
.\build_test_universal.ps1 -Version 7 -Description "fixed alternating rssi" -RevNumber 020
```

## What's Different from v5

V7 combines the working foundation of v5 with the alternating RSSI feature:
- Every other transmission sends fixed -55 dBm (with LED3 flash)
- Alternate transmissions send simulated real RSSI (with LED2 flash)
- All LED controls work properly
- Heartbeat confirms firmware is running

## Troubleshooting

If LED0 doesn't blink:
1. Check power to the board
2. Verify the hex file was flashed correctly
3. Try a full chip erase before flashing

If BLE doesn't advertise:
1. Ensure LED0 is blinking first (confirms firmware is running)
2. Check BLE scanner is working with other devices
3. Look for "SinglePing Host" in the scanner

The v7 firmware has been thoroughly reviewed and uses the proven working structure from v5.
