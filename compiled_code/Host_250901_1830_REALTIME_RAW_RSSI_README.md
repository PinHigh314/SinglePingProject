# Host Firmware - Real-time Raw RSSI Transmission
## Version: Host_250901_1830_REALTIME_RAW_RSSI.hex

### Major Changes Made

#### 1. Fixed Mipe Status Auto-transmission Issue
- **REMOVED**: Automatic Mipe status updates every 5 seconds from main loop
- **FIXED**: Mipe status is now ONLY sent when explicitly requested via MIPE_SYNC command (Sync button)
- This prevents unwanted periodic status transmissions

#### 2. Changed RSSI Data Transmission to Raw Format
- **BEFORE**: 4-byte packet (RSSI + 24-bit timestamp)
- **AFTER**: 1-byte raw RSSI value only
- Simplified data format for cleaner transmission
- No timestamp packaging - just the raw signed int8 RSSI value

#### 3. Implemented Real-time RSSI Transmission
- **REMOVED**: 10Hz timer-based transmission (100ms intervals)
- **ADDED**: Real-time forwarding - RSSI sent immediately when received from Mipe
- RSSI is now sent directly from the `mipe_rssi_received()` callback
- Data rate matches actual beacon reception rate (~10 beacons/second)

#### 4. Data Flow Summary
```
Mipe Beacon (100ms) → Host receives → Immediate forward to App
                      ↓
                   Raw RSSI byte
                   (no packaging)
```

### Key Benefits
1. **Real-time data**: No 100ms delay from timer
2. **Raw data**: App receives unmodified RSSI values
3. **Cleaner separation**: RSSI stream vs Mipe status are clearly separated
4. **No auto-updates**: Mipe status only on demand

### Testing Notes
- RSSI data will arrive at ~10Hz (matching Mipe beacon rate)
- Each notification contains only 1 byte (signed RSSI value)
- Mipe status will only update when Sync button is pressed
- App graph should update smoothly with real-time data

### Technical Details
- **RSSI Format**: Single signed int8_t byte (-128 to +127 dBm)
- **Transmission**: BLE GATT notifications on RSSI characteristic
- **Rate**: Matches Mipe advertising interval (~100ms)
- **Mipe Status**: 16-byte formatted packet (only on MIPE_SYNC command)

### Build Info
- Built with nRF Connect SDK v3.1.0
- Board: nrf54l15dk/nrf54l15/cpuapp
- Memory usage: FLASH: 21.23%, RAM: 42.59%
- Size: 856.54 KB

### Files Modified
1. `Host/host_device/src/main.c`
   - Removed automatic `update_mipe_status()` from main loop
   - Added real-time RSSI forwarding in `mipe_rssi_received()`
   
2. `Host/host_device/src/ble/ble_peripheral.c`
   - Removed timer-based transmission
   - Changed `ble_peripheral_send_rssi_data()` to send raw byte only
   - Removed timestamp packaging logic
