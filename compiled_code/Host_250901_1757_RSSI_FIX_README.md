# Host Firmware - RSSI Acquisition Fix
## Version: Host_250901_1757.hex

### Problem Fixed
The Host was not receiving real RSSI data from the Mipe device because it was filtering out non-connectable beacon advertisements. This caused erratic RSSI values as the Host picked up other BLE devices or noise instead of the actual Mipe beacon.

### Key Changes Made

#### 1. Fixed Beacon Type Filtering (ble_central.c)
- **REMOVED**: Restrictive filter that only accepted connectable advertising types
- **ADDED**: Support for ALL advertising types including `BT_GAP_ADV_TYPE_ADV_NONCONN_IND` (non-connectable beacons)
- This allows the Host to properly detect Mipe beacons which advertise as non-connectable

#### 2. Enhanced Logging for Mipe Detection (ble_central.c)
- Added "*** SinglePing Mipe DETECTED ***" message when first found
- Shows "Connection to Mipe: CONNECTED (Beacon Mode)" status
- Logs RSSI updates every 10 packets received
- Tracks total packet count from Mipe
- Implements 10-second timeout detection with "*** SinglePing Mipe LOST ***" warning

#### 3. Connection State Tracking (main.c)
- Added periodic status logging showing:
  - Connection duration in seconds
  - Total packets received
  - Current RSSI value
- Improved connection/disconnection state change notifications
- Better handling of beacon timeout scenarios

#### 4. Logging Level Changes
- Changed ble_central module from LOG_LEVEL_WRN to LOG_LEVEL_INF for better visibility

### Expected Behavior After Fix
1. Host will now properly detect "SinglePing Mipe" beacons
2. RSSI values should be stable around -43 dBm (matching phone scanner readings)
3. No more erratic spikes in RSSI data
4. Clear logging of Mipe detection and connection status
5. Automatic disconnection detection after 10 seconds of no beacons

### Testing Instructions
1. Flash this firmware to the Host device
2. Ensure Mipe device is advertising as "SinglePing Mipe"
3. Monitor serial output for:
   - "*** SinglePing Mipe DETECTED ***" message
   - Stable RSSI readings without spikes
   - Periodic status updates every 5 seconds

### Build Info
- Built with nRF Connect SDK v3.1.0
- Board: nrf54l15dk/nrf54l15/cpuapp
- Memory usage: FLASH: 21.23%, RAM: 42.59%
