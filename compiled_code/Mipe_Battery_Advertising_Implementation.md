# Mipe Battery Voltage in BLE Advertising Implementation

## Date: September 2, 2025

## Overview
Successfully implemented battery voltage transmission in Mipe's BLE advertising packets, allowing the Host device to read battery status without establishing a connection.

## Implementation Details

### 1. Mipe Device Changes

#### ble_service.c Modifications:
- Added dynamic advertising data structure with manufacturer data field
- Battery voltage transmitted as 2-byte value in manufacturer data
- Format: `[Company ID (2 bytes)][Battery mV (2 bytes)]`
- Company ID set to 0xFFFF for testing purposes
- Battery voltage updated in advertising data at boot and after disconnection
- Periodic updates every 30 seconds when not connected

#### Key Functions Added:
```c
static void update_advertising_data(void)
{
    uint16_t battery_mv = battery_monitor_get_voltage_mv();
    mfg_data[2] = battery_mv & 0xFF;           /* Low byte */
    mfg_data[3] = (battery_mv >> 8) & 0xFF;    /* High byte */
    ad[2] = (struct bt_data)BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data));
}
```

### 2. Host Device Changes

#### ble_central.c Modifications:
- Enhanced advertising data parser to extract manufacturer data
- Added battery voltage tracking variable
- Updated logging to show both RSSI and battery voltage
- Battery displayed every 10 packets alongside RSSI

#### New Function Added:
```c
uint16_t ble_central_get_mipe_battery_mv(void)
```
Returns the last received battery voltage from Mipe's advertising packets.

### 3. Data Flow

1. **Mipe Side:**
   - Battery monitor initialized at boot
   - Battery voltage read and included in advertising packets
   - Updates every 30 seconds when advertising
   - Fresh battery reading on each advertising restart

2. **Host Side:**
   - Scans for device named "SinglePing Mipe"
   - Parses advertising data for manufacturer field
   - Extracts battery voltage from bytes 2-3 of manufacturer data
   - Logs RSSI and battery together for monitoring

## Technical Specifications

### Advertising Packet Structure:
```
- Flags: BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR
- UUID: 87654321-4321-8765-4321-987654321098
- Manufacturer Data: [0xFF, 0xFF, Battery_Low, Battery_High]
- Device Name (in scan response): "SinglePing Mipe"
```

### Battery Voltage Format:
- 16-bit unsigned integer
- Little-endian byte order
- Range: 0-65535 mV
- Typical values: 2800-3300 mV for CR2032

## Power Considerations

- Battery reading adds minimal overhead
- ADC initialized once at boot
- Readings cached for 30 seconds during advertising
- No impact on connection-based operations

## Testing Notes

1. **Verify Battery in Advertising:**
   - Build and flash Mipe device
   - Monitor Host serial output
   - Should see: "Mipe Update: RSSI=-XX dBm, Battery=XXXX mV"

2. **Expected Behavior:**
   - Battery voltage appears in Host logs every 10 packets
   - Value should be realistic (2800-3300 mV for CR2032)
   - Updates every 30 seconds when Mipe is advertising

## Build Commands

```bash
# Build Mipe with battery in advertising
cd BAT\ files
./build_mipe.bat

# Build Host with battery parsing
./build_host.bat
```

## Future Enhancements

1. Add battery level threshold alerts
2. Include battery percentage calculation
3. Implement low battery warning in Host
4. Add battery trend tracking
5. Consider power-save mode when battery is low

## Files Modified

- `Mipe/mipe_device/src/ble_service.c` - Added battery to advertising
- `Mipe/mipe_device/src/battery_monitor.c` - Boot-time initialization
- `Mipe/mipe_device/src/main.c` - Restored battery_monitor_init()
- `Host/host_device/src/ble/ble_central.c` - Parse battery from advertising
- `Host/host_device/src/ble/ble_central.h` - Added battery getter function

## Status: COMPLETE ✓

Battery voltage is now successfully transmitted in Mipe's advertising packets and parsed by the Host device.
