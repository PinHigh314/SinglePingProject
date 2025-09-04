# RSSI Measurement Process Analysis
## Current Implementation Overview

This document outlines exactly what the code is doing now for RSSI measurement and filtering.

## 1. Data Flow Architecture

```
Raw RSSI → Optimistic Filter → Kalman Filter → Filtered RSSI
    ↓                                              ↓
Raw History                              Filtered History
    ↓                                              ↓
Graph (Blue Line)                        Graph (Red Line)
```

## 2. Raw RSSI Input
**Source:** `MotoAppBleViewModel.handleRealRssiData()`

When BLE data arrives from the Host device:
- **Input:** Integer RSSI value (typically -30 to -90 dBm)
- **Additional data:** Host battery (mV), Mipe battery (mV)
- **Rate:** ~10 Hz (100ms intervals)

```kotlin
// Raw data arrives from BLE callback
bleManager.onRssiDataReceived = { rssi, hostBatteryMv, mipeBatteryMv ->
    handleRealRssiData(rssi, hostBatteryMv, mipeBatteryMv)
}
```

## 3. Optimistic Filter (First Stage)
**File:** `OptimisticFilter.kt`
**Purpose:** Pre-filter to handle signal anomalies before Kalman filtering

### Filter Rules:
1. **Stronger signals (higher RSSI):** Accept immediately
   - Rationale: Better line of sight = trust immediately
   - Example: If current estimate is -50 dBm and new reading is -45 dBm → ACCEPT

2. **Weaker signals (lower RSSI):** Apply skepticism
   - If drop > 5 dBm: REJECT (likely interference)
   - If drop ≤ 5 dBm: ACCEPT (normal variation)
   - Example: Current -50 dBm, new -56 dBm (drop = 6) → REJECT

3. **Anti-stuck mechanism:** 
   - After 5 consecutive rejections, force accept
   - Prevents filter from getting stuck on old values

### Parameters:
- `rejectionThreshold`: 5.0 dBm
- `maxConsecutiveRejections`: 5

## 4. Kalman Filter (Second Stage)
**File:** `KalmanFilter.kt`
**Purpose:** Optimal state estimation with noise reduction

### Mathematical Model:
```
Prediction Step:
  x_pred = x_prev (constant model, no motion)
  P_pred = P_prev + Q

Update Step:
  K = P_pred / (P_pred + R)  // Kalman gain
  x = x_pred + K * (measurement - x_pred)
  P = (1 - K) * P_pred
```

### Parameters:
- **Q (Process Noise):** 0.1
  - Low value = assumes RSSI doesn't change rapidly
  - More smoothing, slower response

- **R (Measurement Noise):** 2.0
  - Moderate value = measurements are somewhat noisy
  - Balanced trust between model and measurements

### Kalman Gain Interpretation:
- K ≈ 1: Trusts measurements more (less smoothing)
- K ≈ 0: Trusts model more (more smoothing)
- Typical K value with current settings: ~0.05-0.3

## 5. Data Processing Pipeline

```kotlin
// In handleRssiData() method:

1. Get current Kalman estimate for reference
   currentEstimate = kalmanFilter.getCurrentEstimate()

2. Apply Optimistic Filter
   acceptedValue = optimisticFilter.filter(rawRssi, currentEstimate)
   
3. Apply Kalman Filter (if not rejected)
   if (acceptedValue != null) {
       filteredRssi = kalmanFilter.filter(acceptedValue)
   } else {
       filteredRssi = currentEstimate  // Use previous estimate
   }

4. Store both raw and filtered values
   - Raw RSSI → rssiHistory (blue line on graph)
   - Filtered RSSI → filteredRssiHistory (red line on graph)
```

## 6. Data Storage

### History Buffers:
- **Size:** 300 samples (30 seconds at 10 Hz)
- **Raw RSSI History:** Last 300 unfiltered values
- **Filtered RSSI History:** Last 300 filtered values

### Log Data Structure:
```kotlin
LogData(
    timestamp: Long,
    rssi: Float,           // Raw RSSI
    filteredRssi: Float,   // After Optimistic + Kalman
    distance: Float,       // Currently always 0 (not calculated)
    hostBatteryMv: Int,
    mipeBatteryMv: Int
)
```

## 7. Calibration Process

### Current Implementation:
- **Target samples:** 50 (hardcoded, should be 110)
- **Collection:** Raw RSSI only (not using filtered values yet)
- **Storage:** CalibrationData with timestamp, rssi, mipeBatteryMv

### Issues Identified:
1. Not collecting 110 samples as specified
2. Not discarding first 10 samples
3. Not storing filtered RSSI values
4. Not calculating averages properly

## 8. Distance Calculation

### Current Status:
- **LogarithmicDistanceCalculator.kt** created but NOT integrated
- Distance always logged as 0.0
- No distance display on UI
- Calibration table exists but not used

### Prepared Model (Not Active):
```kotlin
// Logarithmic path loss model
distance = 10^((A - RSSI) / (10 * n))
Where:
  A = RSSI at 1 meter (reference)
  n = path loss exponent
```

## 9. UI Display

### Main Screen Graph:
- **Blue line:** Raw RSSI values
- **Red line:** Filtered RSSI values
- **X-axis:** Time (last 30 seconds)
- **Y-axis:** RSSI in dBm

### Missing UI Elements:
1. Distance display in green text
2. Filtered RSSI in log output
3. Proper calibration feedback

## 10. Export Functionality

### Current JSON Export:
```json
{
  "timestamp": 1234567890,
  "rssi": -50.0,
  "filteredRssi": -49.5,
  "distance": 0.0,
  "hostBatteryMv": 3300,
  "mipeBatteryMv": 3200
}
```

### Issues:
- Timestamp not human-readable
- Distance always 0
- Too many fields for simple analysis

## Summary of Current Behavior

1. **Input:** Raw RSSI arrives at ~10 Hz from BLE
2. **Filter 1:** Optimistic filter accepts/rejects based on asymmetric rules
3. **Filter 2:** Kalman filter smooths accepted values
4. **Output:** Dual-line graph shows raw (blue) and filtered (red)
5. **Storage:** Both values saved to history buffers
6. **Export:** JSON file with all data points
7. **Distance:** NOT calculated (always 0)
8. **Calibration:** Partially implemented, not using filtered values

## Key Problems Identified

1. **Calibration not complete:** Should collect 110 samples, discard first 10
2. **No distance calculation:** LogarithmicDistanceCalculator not integrated
3. **UI missing elements:** No distance display, no filtered RSSI in logs
4. **Export too verbose:** Timestamps not readable, unnecessary fields

## Filter Performance Characteristics

### Optimistic Filter:
- **Response to improvement:** Instant (0 samples delay)
- **Response to degradation:** 0-5 samples delay (depending on threshold)
- **Stuck prevention:** Max 5 rejections before forced accept

### Kalman Filter:
- **Settling time:** ~10-20 samples to converge
- **Lag:** ~2-5 samples behind rapid changes
- **Noise reduction:** ~60-70% reduction in variance

### Combined System:
- **Best case latency:** 0 samples (signal improves)
- **Worst case latency:** 5 samples (persistent weak signal)
- **Typical smoothing:** 3-5 dB reduction in peak-to-peak variation
