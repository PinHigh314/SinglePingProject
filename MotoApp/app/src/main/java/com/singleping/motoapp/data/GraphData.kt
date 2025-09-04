package com.singleping.motoapp.data

/**
 * Data class for tracking Stage 2 distance output for graphing
 */
data class DistancePoint(
    val timestamp: Long,
    val distance: Float,
    val rssiValue: Float  // The RSSI value that produced this distance
)

/**
 * Combined graph data for dual-axis plotting
 */
data class GraphData(
    val rssiHistory: List<RssiData> = emptyList(),
    val distanceHistory: List<DistancePoint> = emptyList()
)
