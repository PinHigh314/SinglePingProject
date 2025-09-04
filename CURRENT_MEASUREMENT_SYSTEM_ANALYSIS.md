# Current Measurement System Analysis
## As of September 4, 2025

## Overview
The measurement system has been simplified to show raw RSSI data with optional Kalman filtering. All distance calculations and complex filtering have been removed.

## Data Flow Pipeline

### 1. Data Reception (Host Device → Android App)
**Source:** `HostBleManager.onRssiDataReceived` callback

When BLE data arrives from the Host device:
- **Raw RSSI value** (integer, typically -20 to -100 dBm)
- **Host battery voltage** (in millivolts)
- **Mipe battery voltage** (in millivolts)

### 2. ViewModel Processing (`MotoAppBleViewModel`)

#### 2.1 Raw Data Handling (`handleRealRssiData`)
```kotlin
handleRealRssiData(rssi: Int, hostBatteryMv: Int, mipeBatteryMv: Int)
```
- Receives raw RSSI as integer
- Converts to float for processing
- Timestamps each measurement
- Routes to main processing function

#### 2.2 Main Processing (`handleRssiData`)
```kotlin
handleRssiData(rssiValue: Float, timestamp: Long, hostBatteryMv: Int, mipeBatteryMv: Int)
```

**Step 1: Store Raw RSSI**
- Creates `RssiData` object with:
  - `timestamp`: Current system time in milliseconds
  - `value`: Raw RSSI value (float, no manipulation)
  - `hostBatteryMv`: Host battery in millivolts
  - `mipeBatteryMv`: Mipe battery in millivolts
- Adds to `_rssiHistory` list
- Keeps last 300 values (30 seconds at 10Hz sampling rate)

**Step 2: Apply Kalman Filter**
- Passes raw RSSI through 1D Kalman filter
- Filter parameters:
  - Process noise (Q) = 0.1f (allows some variation)
  - Measurement noise (R) = 2.0f (RSSI measurements are noisy)
- Creates filtered `RssiData` object with same structure
- Stores in `_filteredRssiHistory` list
- Also keeps last 300 values

**Step 3: Update Statistics**
- Increments packet counter
- Updates host info with current RSSI and battery
- Logs data for export (if streaming active)

### 3. Kalman Filter Implementation (`KalmanFilter.kt`)

The 1D Kalman filter works as follows:

**Initialization:**
- First measurement initializes state to that value
- Error covariance starts at 1.0

**For each subsequent measurement:**

1. **Prediction Step:**
   - State prediction: `xPred = x` (assumes constant model)
   - Error covariance prediction: `pPred = p + Q`

2. **Update Step:**
   - Calculate Kalman gain: `K = pPred / (pPred + R)`
   - Update state: `x = xPred + K * (measurement - xPred)`
   - Update error covariance: `p = (1 - K) * pPred`

3. **Return filtered value**

The Kalman gain (K) determines how much to trust new measurements:
- K close to 1 → Trust measurements more (less smoothing)
- K close to 0 → Trust model more (more smoothing)

### 4. Graph Display (`RssiGraph.kt`)

The graph component displays both raw and filtered RSSI:

**Graph Configuration:**
- Size: Full width × 250dp height
- Background: Light gray (#F5F5F5)
- Time window: 30 seconds (300 samples)

**Y-Axis Scaling:**
- Left axis: RSSI in dBm (-20 to -100)
- Right axis: Distance placeholder (0 to 160m, not used)
- Scale range: 80 dB (-20 to -100)
- 9 horizontal grid lines

**Data Plotting:**

1. **Raw RSSI (Blue line):**
   - Color: Blue with 50% transparency
   - Line width: 1.5dp
   - Shows actual received values without modification

2. **Filtered RSSI (Red line):**
   - Color: Red with 100% opacity
   - Line width: 2dp
   - Shows Kalman filtered values
   - Last 5 points marked with dots

**Coordinate Calculation:**
```kotlin
// X-axis: Time (0 to 299 samples)
x = (index / 299f) * width

// Y-axis: RSSI value
normalizedRssi = (rssiValue - (-100)) / 80
y = height * (1 - normalizedRssi)
```

### 5. Data Export

When logging is active, each measurement is saved with:
- Raw RSSI value
- Distance: 0 (placeholder, no calculation)
- Host battery voltage
- Mipe battery voltage
- Timestamp

Export creates CSV file in Downloads folder with all logged data.

## Key Points About Current Implementation

### What IS Happening:
1. **Raw RSSI values displayed directly** - No manipulation or offset
2. **Optional Kalman filtering** - Simple 1D filter for smoothing
3. **Dual display** - Both raw (blue) and filtered (red) shown simultaneously
4. **Battery monitoring** - Both Host and Mipe battery levels tracked
5. **Data logging** - All measurements saved for export

### What is NOT Happening:
1. **No distance calculation** - Distance always shows 0
2. **No complex filtering** - Removed all multi-stage filters
3. **No clustering** - Removed RssiClusteringFilter
4. **No velocity smoothing** - Removed VelocitySmoothing
5. **No histogram analysis** - Removed ImprovedDistanceCalculator
6. **No calibration lookup** - Not using rssi_to_distance_lookup.json
7. **No 5dB offset** - Graph now correctly aligned with axis labels

## Potential Issues

### 1. Graph Scaling
The graph Y-axis is fixed from -20 to -100 dBm. If RSSI values fall outside this range:
- Values above -20 dBm will appear at the top
- Values below -100 dBm will appear at the bottom

### 2. Buffer Size
Both raw and filtered histories keep 300 samples. At 10Hz sampling:
- Shows last 30 seconds of data
- Older data is discarded

### 3. Kalman Filter Tuning
Current parameters (Q=0.1, R=2.0) provide moderate smoothing:
- May need adjustment based on actual noise characteristics
- Can be modified in ViewModel initialization

## Summary

The current system is a straightforward RSSI measurement and display system:
1. Receives raw RSSI from Host device
2. Stores it without modification
3. Optionally applies simple Kalman filtering
4. Displays both raw and filtered values on a graph
5. Exports data for analysis

All complex processing has been removed to provide a clear view of the actual measured RSSI values.
