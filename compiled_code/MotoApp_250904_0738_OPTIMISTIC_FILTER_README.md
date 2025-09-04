# MotoApp Build - Optimistic Filter Implementation
**Build Date:** September 4, 2025, 07:38  
**APK File:** MotoApp_250904_0738_OPTIMISTIC_FILTER.apk

## Key Changes

### Added Optimistic Filter for RSSI Pre-processing

This build introduces an "optimistic filter" that pre-processes RSSI values before they reach the Kalman filter. This improves distance measurement stability and responsiveness.

## Implementation Details

### 1. New OptimisticFilter Class
- **Location:** `MotoApp/app/src/main/java/com/singleping/motoapp/distance/OptimisticFilter.kt`
- **Purpose:** Apply asymmetrical filtering rules to RSSI values
- **Default Rejection Threshold:** 5.0 dBm

### 2. Filtering Logic

The optimistic filter applies these rules:

#### When Signal Gets Stronger (raw_rssi > current_estimate):
- **Action:** Accept immediately
- **Rationale:** Stronger signals indicate better line of sight
- **Result:** Quick response to improved conditions

#### When Signal Gets Weaker (raw_rssi < current_estimate):
- Calculate delta = current_estimate - raw_rssi
- If delta > 5 dBm:
  - **Action:** Reject the reading
  - **Rationale:** Likely a reflection or interference
  - **Result:** Maintains stability
- If delta ≤ 5 dBm:
  - **Action:** Accept the reading
  - **Rationale:** Normal signal variation
  - **Result:** Tracks gradual changes

### 3. Processing Pipeline

```
Raw RSSI → Optimistic Filter → Kalman Filter → Distance Calculation
```

1. **Raw RSSI arrives** (every ~100ms from BLE)
2. **Optimistic Filter** decides accept/reject
3. **Kalman Filter** smooths accepted values
4. **Distance** calculated from final filtered value
5. **Graph displays** both raw (blue) and filtered (red) lines

### 4. Modified Files

- **KalmanFilter.kt:** Added `getCurrentEstimate()` method
- **MotoAppBleViewModel.kt:** Integrated optimistic filter in `handleRssiData()`
- **OptimisticFilter.kt:** New file implementing the filter

## Benefits

1. **Improved Stability:** Rejects sudden weak signals from reflections
2. **Better Responsiveness:** Immediately accepts stronger signals
3. **Accurate Distance:** More reliable distance measurements
4. **Maintains Visualization:** Graph still shows raw vs filtered for debugging

## Technical Parameters

- **Optimistic Filter Rejection Threshold:** 5.0 dBm
- **Kalman Filter Process Noise (Q):** 0.1
- **Kalman Filter Measurement Noise (R):** 2.0

## Testing Notes

The optimistic filter should:
- Make distance readings more stable when stationary
- Respond quickly when moving closer (stronger signal)
- Resist jumping to larger distances from temporary reflections
- Work seamlessly with existing Kalman filtering

## Graph Visualization

- **Blue Line:** Raw RSSI values (all readings)
- **Red Line:** Final filtered output (Optimistic + Kalman)
- When a value is rejected, the red line stays flat (uses previous estimate)

## Future Improvements

Consider adding:
- Adjustable rejection threshold in UI
- Visual indicators for rejected readings
- Statistics on rejection rate
- Time-based logic for consecutive rejections
