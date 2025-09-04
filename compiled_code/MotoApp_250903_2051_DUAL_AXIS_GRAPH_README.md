# MotoApp Dual-Axis Graph Implementation
**Version:** MotoApp_250903_2051_DUAL_AXIS_GRAPH.apk  
**Date:** September 3, 2025, 20:51

## Changes Made

### 1. Dual-Axis Graph Implementation
- Added new `DualAxisRssiGraph.kt` component with:
  - **Left Y-axis**: RSSI values in dBm (-20 to -100)
  - **Right Y-axis**: Distance in meters (0.14m to 271m)
  - **Blue line**: Raw RSSI values (10 Hz update rate)
  - **Green line**: Stage 2 distance output (1 Hz after clustering)

### 2. UI Cleanup
- Removed overlapping text labels:
  - "RSSI (dBm)" label
  - "Distance (m)" label  
  - Legend with dots and text
- Kept only the essential axis values for cleaner visualization

### 3. Distance Tracking
- Added `distanceHistory` state to ViewModel
- Tracks Stage 2 output (post-clustering, pre-velocity smoothing)
- Stores up to 30 distance points for graphing

### 4. Data Flow Visualization
The dual-axis graph now shows:
- **RSSI Input**: 10 samples per second (blue line)
- **Distance Output**: 1 point per second after clustering (green line)
- Clear visualization of the clustering buffer effect

## Technical Details

### Distance Calculation Pipeline
1. **Stage 1**: RSSI Clustering (10 samples → 1 clustered value)
2. **Stage 2**: Distance Lookup (using calibrated table) ← **Plotted in green**
3. **Stage 3**: Velocity Smoothing (for display)

### Graph Scaling
- RSSI: Linear scale (-100 to -20 dBm)
- Distance: Logarithmic scale (0.14m to 271m) to match non-linear RSSI-distance relationship

## Testing Notes
- Green distance line should appear after ~1 second of streaming (when clustering completes)
- Debug logging added to track distance point additions
- Check logcat for "DualAxisGraph" and "Stage 2 Distance" messages

## Known Issues to Monitor
- Distance line may not appear if clustering doesn't complete
- Verify that distance points are being added every second during streaming
