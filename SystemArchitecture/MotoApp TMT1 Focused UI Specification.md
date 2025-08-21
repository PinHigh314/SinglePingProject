# MotoApp TMT1 Focused UI Specification

## TMT1 Objective
Build essential MotoApp functionality to connect to Host module and display simulated measurement data. Focus on core display functions only.

---

## Single Main Screen Layout

### 1. Host Connection Section (Top Priority)
**"Connect to Host Module" Button**
- **Text**: "CONNECT TO HOST"
- **Size**: Large, prominent button (full width)
- **Color**: Blue background, white text
- **Position**: Top of screen
- **States**:
  - Enabled: "CONNECT TO HOST" (when disconnected)
  - Connecting: "CONNECTING..." (during connection process)
  - Connected: "DISCONNECT HOST" (when connected)
- **Action**: Initiates BLE scanning and connection to Host device

### 2. Data Stream Control Section
**Start Simulation Button**
- **Text**: "START DATA STREAM"
- **Size**: Medium button (full width)
- **Color**: Green background, white text
- **Position**: Below connection section
- **States**:
  - Disabled: When Host not connected
  - Enabled: "START DATA STREAM" (when Host connected but not streaming)
  - Active: "STOP DATA STREAM" (when streaming active)
- **Action**: Requests Host to begin simulated data transmission

### 3. Real-Time RSSI Graph (Core Display)
**RSSI Histogram Display**
- **Graph Type**: Real-time line chart
- **Y-Axis**: -30 dBm to -80 dBm (fixed range)
- **X-Axis**: Time (last 30 seconds, scrolling window)
- **Update Rate**: Every 100ms
- **Line Color**: Orange/Red
- **Grid**: Light gray grid lines
- **Labels**: 
  - Y-axis: "-30", "-40", "-50", "-60", "-70", "-80" dBm
  - X-axis: Time stamps every 5 seconds
- **Size**: Takes up 40% of screen height

### 4. Status Display Panel
**BLE Connection Status**
- **Host Status**: 
  - Text: "Host: Connected" / "Host: Disconnected" / "Host: Searching"
  - Icon: Bluetooth symbol
  - Color: Green (connected), Red (disconnected), Orange (searching)

**Data Stream Status**
- **Stream Status**: "Data Stream: Active" / "Data Stream: Stopped"
- **Data Rate**: "Update Rate: 100ms"
- **Packets Received**: "Packets: 1,247"

**Host Device Information**
- **Device Name**: "MIPE_HOST_A1B2"
- **Battery Level**: "USB Powered" (Host is always USB powered)
- **Signal Strength**: "-45 dBm" (BLE connection to Host)
- **Connection Time**: "Connected: 00:02:15"

### 5. Distance Calculation Section
**Current Distance Display**
- **Large Numeric Display**: "5.23 m"
- **Font Size**: 48sp
- **Color**: Green (good signal), Orange (weak signal), Red (poor signal)
- **Position**: Prominent, below graph

**Distance Calculation Info**
- **Algorithm Used**: "RSSI-Based (Simulated)"
- **Confidence**: "±0.5m"
- **Last Updated**: "0.1s ago"

**Distance Statistics**
- **Average Distance**: "5.1m"
- **Min Distance**: "4.8m"
- **Max Distance**: "5.6m"
- **Sample Count**: "1,247"

---

## TMT1 Simulated Data Requirements

### RSSI Simulation
- **Range**: -40 dBm to -75 dBm
- **Pattern**: Slowly varying (simulate movement)
- **Noise**: ±2 dBm random variation
- **Update Rate**: Every 100ms
- **Correlation**: Simulate realistic signal strength variations

### Distance Calculation (Simulated)
- **Base Distance**: 5.0m (simulated actual distance)
- **Calculation**: Use RSSI to distance formula (simulated)
- **Formula**: Distance = 10^((Tx_Power - RSSI) / (10 * N))
  - Tx_Power: -20 dBm (simulated)
  - N: 2.0 (path loss exponent)
- **Variation**: ±0.3m realistic noise
- **Update**: Recalculate with each new RSSI value

### Connection Simulation
- **Discovery Time**: 2-3 seconds to find Host
- **Connection Time**: 1-2 seconds to establish connection
- **Connection Stability**: Maintain stable connection during TMT1
- **Reconnection**: Automatic reconnection if connection lost

---

## UI Behavior and States

### App Launch State
1. **Initial State**: All buttons enabled, no data displayed
2. **Status**: "Host: Disconnected", "Data Stream: Stopped"
3. **Graph**: Empty graph with axes labeled
4. **Distance**: "-- m" (no data)

### Connection Process
1. **User taps "CONNECT TO HOST"**
2. **Button changes to "CONNECTING..."**
3. **Status shows "Host: Searching"**
4. **After 2-3 seconds: "Host: Connected"**
5. **Button changes to "DISCONNECT HOST"**
6. **"START DATA STREAM" button becomes enabled**

### Data Streaming Process
1. **User taps "START DATA STREAM"**
2. **Button changes to "STOP DATA STREAM"**
3. **Status shows "Data Stream: Active"**
4. **Graph begins updating with RSSI data**
5. **Distance calculation begins updating**
6. **Statistics begin accumulating**

### Error Handling
- **Connection Failed**: Show "Connection Failed - Retry?" dialog
- **Connection Lost**: Automatically attempt reconnection
- **No Data**: Show "No data received" message after 5 seconds
- **Poor Signal**: Change distance display color to orange/red

---

## Technical Implementation Notes

### BLE Integration Points
- **Service UUID**: Custom UUID for Host identification
- **Characteristics**: 
  - RSSI Data (Notify): Receives simulated RSSI values
  - Control (Write): Send start/stop stream commands
  - Status (Read): Read Host status information

### Data Processing
- **RSSI Buffer**: Keep last 300 values (30 seconds at 100ms)
- **Distance Calculation**: Real-time calculation from RSSI
- **Statistics**: Running averages and min/max tracking
- **Graph Updates**: Efficient UI updates without blocking

### Mock Data for TMT1
Since TMT1 is app-only (no actual Host connection):
- **Simulate BLE discovery**: 2-3 second delay
- **Simulate connection**: 1-2 second delay  
- **Generate RSSI data**: Mathematical function with noise
- **Calculate distance**: Use realistic RSSI-to-distance formula
- **Maintain consistency**: Realistic data patterns

---

## Success Criteria for TMT1

### Functional Requirements
✅ **Connection Button**: Successfully simulates Host connection
✅ **Data Stream Control**: Start/stop simulation works
✅ **RSSI Graph**: Real-time display of simulated RSSI data
✅ **Distance Display**: Calculated distance from simulated RSSI
✅ **Status Panel**: Shows all connection and stream status
✅ **Statistics**: Accurate calculation of min/max/average

### UI/UX Requirements
✅ **Responsive Interface**: Smooth 60fps updates
✅ **Clear Status**: Always obvious what state the app is in
✅ **Professional Appearance**: Clean, readable interface
✅ **Error Handling**: Graceful handling of simulated errors
✅ **Data Visualization**: Clear, readable graph and numbers

### Preparation for TMT2
✅ **BLE Architecture**: Ready to replace simulation with real BLE
✅ **Data Structure**: Compatible with real Host data format
✅ **UI Framework**: Ready to add real connection management
✅ **Error Handling**: Framework ready for real BLE errors

This focused approach gets TMT1 working quickly while building the foundation for TMT2's real BLE integration!

