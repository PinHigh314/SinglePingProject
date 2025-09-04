# Current Measurement System - Complete Flow Analysis

## Overview
The measurement system has been simplified to show raw RSSI data with optional Kalman filtering. All complex filtering mechanisms have been removed.

## Data Flow Architecture

### 1. Data Reception (BLE Layer)
**File:** `MotoApp/app/src/main/java/com/singleping/motoapp/ble/HostBleManager.kt`
- Receives BLE notifications from Host device
- Data packet contains:
  - RSSI value (signal strength in dBm)
  - Host battery voltage (in mV)
  - Mipe battery voltage (in mV)
- Triggers callback: `onRssiDataReceived(rssi, hostBatteryMv, mipeBatteryMv)`

### 2. ViewModel Processing
**File:** `MotoApp/app/src/main/java/com/singleping/motoapp/viewmodel/MotoAppBleViewModel.kt`

#### Raw Data Handling (`handleRealRssiData`)
```kotlin
1. Receives: rssi (Int), hostBatteryMv (Int), mipeBatteryMv (Int)
2. Creates timestamp
3. Checks if in calibration mode (collects 50 samples if active)
4. Updates host battery info
5. Updates Mipe battery info
6. Calls handleRssiData() for further processing
```

#### RSSI Processing (`handleRssiData`)
```kotlin
1. Creates RssiData object with raw RSSI value
2. Stores in _rssiHistory (keeps last 300 values = 30 seconds at 100ms rate)
3. Applies Kalman filter to raw RSSI
4. Stores filtered value in _filteredRssiHistory (also 300 values)
5. Updates packet counter
6. Logs data if streaming is active
```

#### Kalman Filter Configuration
```kotlin
private val kalmanFilter = KalmanFilter(
    processNoise = 0.1f,      // Q: Low value = trusts model more
    measurementNoise = 2.0f   // R: Higher value = trusts measurements less
)
```

The Kalman filter smooths RSSI values by:
- Predicting next value based on previous state
- Updating prediction with new measurement
- Balancing between prediction and measurement based on noise parameters

#### Data Logging (`logData`)
```kotlin
1. Gets latest filtered RSSI value
2. Creates LogData object containing:
   - Raw RSSI
   - Filtered RSSI
   - Distance (currently set to 0, not calculated)
   - Host battery info
   - Mipe battery info
   - Battery values in mV
3. Stores in _loggingData (keeps last 1000 samples)
4. Updates statistics
```

### 3. UI Display Layer
**File:** `MotoApp/app/src/main/java/com/singleping/motoapp/ui/screens/MainScreen.kt`

#### Graph Display
- Shows dual-line graph:
  - **Blue line (semi-transparent):** Raw RSSI values
  - **Red line (solid):** Kalman filtered RSSI values
- Y-axis: RSSI in dBm (typically -30 to -100)
- X-axis: Time (last 30 seconds of data)
- Updates in real-time as data arrives

#### Distance Calculation and Display
```kotlin
// In MainScreen.kt
val latestFilteredRssi = filteredRssiHistory.lastOrNull()?.value ?: -50f
val distance = calculateDistance(latestFilteredRssi)

// calculateDistance function uses lookup table
fun calculateDistance(rssi: Float): Float {
    // Uses calibration table from rssi_to_distance_lookup.json
    // Interpolates between known RSSI-distance pairs
    // Returns distance in meters
}
```

Distance is displayed as large green text: "Distance: XX.X m"

#### Real-time Updates
- Connection status and duration
- Packet counter
- Battery voltages (Host and Mipe)
- RSSI graph with both raw and filtered values
- Distance estimate based on filtered RSSI

### 4. Data Export
**File:** `MotoApp/app/src/main/java/com/singleping/motoapp/export/DataExporter.kt`

#### JSON Export Format (Simplified)
```json
{
  "Mipe rssi": -65.0,
  "filteredRssi": -64.5,
  "distance": 2.3,
  "hostBatteryMv": 3300,
  "mipeBatteryMv": 3250,
  "timestamp": "4.9.25_6.20.12"
}
```

#### CSV Export Format
```csv
Timestamp,RSSI,FilteredRSSI,Distance,HostBattery,MipeBattery
2025-09-04 06:20:12,-65.0,-64.5,2.3,3.3,3.25
```

## Key Changes from Previous Version

### Removed Components:
1. **ImprovedDistanceCalculator** - Complex multi-filter system removed
2. **RssiClusteringFilter** - Statistical clustering removed
3. **VelocitySmoothing** - Velocity-based smoothing removed
4. **Histogram-based filtering** - Removed
5. **Adaptive filtering** - Removed
6. **Complex distance calculations** - Simplified to lookup table

### Current Simple Flow:
```
BLE Data → Raw RSSI → Kalman Filter → Filtered RSSI → Distance Lookup → Display
```

### What Each Value Means:
- **Raw RSSI:** Direct signal strength from BLE hardware (-30 to -100 dBm)
- **Filtered RSSI:** Kalman smoothed value for stability
- **Distance:** Estimated distance in meters from lookup table
- **Battery values:** Voltage in millivolts (mV)

## Calibration Mode
When activated:
1. Collects exactly 50 RSSI samples
2. Stores with selected distance value
3. Exports for calibration table updates
4. Used to improve distance lookup accuracy

## Performance Characteristics
- **Update rate:** ~10 Hz (100ms intervals)
- **Graph window:** 30 seconds (300 samples)
- **Log buffer:** 1000 samples maximum
- **Latency:** < 50ms from BLE reception to UI update
- **Memory usage:** Minimal with fixed-size buffers

## Summary
The current system prioritizes:
1. **Transparency** - Shows actual raw RSSI values
2. **Simplicity** - Single Kalman filter for smoothing
3. **Real-time** - Immediate display of measurements
4. **Accuracy** - Uses calibrated lookup table for distance
5. **Debugging** - Logs both raw and filtered values

The measurement is now straightforward: Raw RSSI → Optional Kalman smoothing → Distance lookup → Display
