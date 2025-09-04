# Complete Measurement Flow Analysis

## Overview
The measurement system has multiple stages of processing that transform raw RSSI values into distance measurements. Here's exactly what's happening at each stage:

## Data Flow Pipeline

### Stage 1: Raw RSSI Input (10 Hz)
- **Source**: BLE advertising packets from Mipe device
- **Rate**: ~10 packets per second (100ms intervals)
- **Values**: Raw RSSI in dBm (typically -20 to -100)
- **Location**: `MotoAppBleViewModel.handleRealRssiData()`

### Stage 2: RSSI Clustering (1 Hz output)
- **Purpose**: Filter out multipath reflections and interference
- **Process**: 
  1. Collects 10 RSSI samples into a buffer
  2. Groups samples into clusters with 10% variation tolerance
  3. Selects the majority cluster (most samples)
  4. Outputs the cluster center value
- **Update Rate**: Every 1 second (after 10 samples collected)
- **Location**: `RssiClusteringFilter.processSamples()`
- **Key Parameters**:
  - `clusteringSampleSize = 10` samples
  - `variationPercent = 10%` tolerance
  - `maxClusters = 10` maximum clusters

### Stage 3: Distance Lookup (1 Hz)
- **Purpose**: Convert clustered RSSI to distance using calibrated lookup table
- **Process**:
  1. Takes clustered RSSI value
  2. Looks up distance in calibrated table
  3. Interpolates between table entries
- **Lookup Table Range**: -20 dBm (0.14m) to -100 dBm (271m)
- **Location**: `DistanceLookupTable.getDistance()`

### Stage 4: Velocity Smoothing (1 Hz)
- **Purpose**: Ensure physically realistic movement
- **Process**:
  1. Checks if distance change is physically possible
  2. Limits velocity to realistic human speeds
  3. Applies adaptive smoothing based on change magnitude
- **Velocity Limits**:
  - Walking: 1.4 m/s
  - Brisk Walking: 2.0 m/s
  - Running: 3.5 m/s
  - Max Jump: 5.0 m
- **Smoothing Factors** (adaptive):
  - Very large changes (>3m): 10% smoothing
  - Large changes (>1.5m): 15% smoothing
  - Medium changes (>0.5m): 20% smoothing
  - Small changes (>0.2m): 30% smoothing
  - Tiny changes (<0.2m): 40% smoothing
- **Location**: `VelocitySmoothing.processDistance()`

## Current Issues

### 1. Distance Update Timing
```kotlin
// In MotoAppBleViewModel.handleRssiData()
private val DISTANCE_UPDATE_INTERVAL_MS = 1000L // Updates every 1 second

if (distanceResult != null && shouldUpdateDistance) {
    // Updates distance display and history
    lastDistanceUpdateTime = currentTime
    
    // Stage 2 output (post-clustering, pre-velocity smoothing)
    val stage2Distance = distanceResult.distance
    
    // Add to distance history for graphing
    val distancePoint = DistancePoint(
        timestamp = currentTime,
        distance = stage2Distance,  // This is Stage 2 output
        rssiValue = distanceResult.filteredRssi
    )
}
```

### 2. Buffer Management Issue
```kotlin
// In ImprovedDistanceCalculator.processRssiValue()
if (rssiBuffer.size >= clusteringSampleSize) {
    val result = processBufferedSamples()
    
    // PROBLEM: This clears most of the buffer after processing
    val keepSamples = clusteringSampleSize / 4  // Only keeps 2-3 samples
    rssiBuffer.clear()
    rssiBuffer.addAll(rssiBuffer.takeLast(keepSamples))  // BUG: takeLast on cleared buffer
    
    result
}
```
**Issue**: The buffer is cleared before trying to keep the last samples, resulting in an empty buffer.

### 3. Graph Display
The dual-axis graph shows:
- **Blue line (left axis)**: Raw RSSI values at 10 Hz
- **Green line (right axis)**: Stage 2 distance (post-clustering, pre-smoothing) at 1 Hz

## Data Rate Summary

| Stage | Input Rate | Output Rate | Delay |
|-------|------------|-------------|-------|
| Raw RSSI | 10 Hz | 10 Hz | 0ms |
| Clustering | 10 Hz | 1 Hz | 1000ms |
| Distance Lookup | 1 Hz | 1 Hz | 0ms |
| Velocity Smoothing | 1 Hz | 1 Hz | 0ms |
| Display Update | 1 Hz | 1 Hz | 0ms |

## Key Observations

1. **1-Second Latency**: Distance updates only occur every second due to clustering buffer requirement
2. **Double Smoothing**: Both clustering (averaging within clusters) and velocity smoothing are applied
3. **Stage 2 Graphing**: The green line shows Stage 2 output (post-clustering, pre-velocity smoothing)
4. **Buffer Bug**: The buffer management has a bug that clears samples incorrectly

## Confidence Calculation

Confidence is calculated from two sources:
1. **Clustering confidence** (70% weight):
   - Based on cluster dominance (majority size / total samples)
   - Bonus for fewer clusters (cleaner signal)
2. **Distance confidence** (30% weight):
   - Based on lookup table interpolation quality

## Recommended Fixes

1. **Fix Buffer Management**:
```kotlin
// Correct implementation
if (rssiBuffer.size >= clusteringSampleSize) {
    val result = processBufferedSamples()
    val keepSamples = clusteringSampleSize / 4
    val samplesToKeep = rssiBuffer.takeLast(keepSamples)  // Save before clearing
    rssiBuffer.clear()
    rssiBuffer.addAll(samplesToKeep)  // Now add the saved samples
    result
}
```

2. **Reduce Latency**:
   - Consider reducing cluster size to 5 samples (500ms updates)
   - Or implement sliding window clustering

3. **Improve Responsiveness**:
   - Increase velocity limits for known scenarios
   - Reduce smoothing factors for faster response

4. **Debug Logging**:
   - The comprehensive debug logging in DualAxisRssiGraph should show:
     - Canvas dimensions
     - Data sizes
     - Value ranges
     - Individual point coordinates
     - This will make any graphing issues "dead obvious"
