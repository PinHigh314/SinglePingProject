package com.singleping.motoapp.data

import androidx.compose.ui.graphics.Color
import com.singleping.motoapp.distance.ImprovedDistanceCalculator

// Data classes for MotoApp TMT1

data class ConnectionState(
    val isConnected: Boolean = false,
    val isConnecting: Boolean = false,
    val deviceName: String = "MIPE_HOST_A1B2",
    val connectionTime: Long = 0L // Connection start time in milliseconds
)

data class StreamState(
    val isStreaming: Boolean = false,
    val packetsReceived: Int = 0,
    val updateRate: Int = 100 // milliseconds
)

data class RssiData(
    val timestamp: Long,
    val value: Float, // in dBm
    val hostBatteryMv: Int = 0, // Host battery in millivolts
    val mipeBatteryMv: Int = 0  // Mipe battery in millivolts
)

data class DistanceData(
    val currentDistance: Float = 0f,
    val averageDistance: Float = 0f,
    val minDistance: Float = Float.MAX_VALUE,
    val maxDistance: Float = 0f,
    val confidence: Float = 0.5f, // ±0.5m
    val lastUpdated: Long = 0L,
    val sampleCount: Int = 0
)

data class HostInfo(
    val deviceName: String = "MIPE_HOST_A1B2",
    val batteryLevel: String = "USB Powered",
    val signalStrength: Float = -45f, // dBm
    val batteryVoltage: Float = 0f // Battery voltage in volts
)

// Logging data classes
data class LogData(
    val timestamp: Long = System.currentTimeMillis(),
    val rssi: Float,
    val filteredRssi: Float = 0f, // Kalman filtered RSSI
    val distance: Float,
    val hostInfo: HostInfo,
    val mipeStatus: MipeStatus?,
    val hostBatteryMv: Int = 0, // Host battery in millivolts
    val mipeBatteryMv: Int = 0  // Mipe battery in millivolts
)

data class LogStats(
    val totalSamples: Int = 0,
    val samplesPerSecond: Float = 0f,
    val startTime: Long = 0L,
    val isLogging: Boolean = false
)

data class CalibrationData(
    val timestamp: Long,
    val rssi: Float,
    val filteredRssi: Float = 0f,  // Added filtered RSSI
    val mipeBatteryMv: Int
)

data class CalibrationState(
    val selectedDistance: Int = 0,
    val sampleCount: Int = 0,
    val targetSampleCount: Int = 110,  // Collect 110, discard first 10
    val isCollecting: Boolean = false,
    val isComplete: Boolean = false,
    val comment: String = "",
    val data: List<CalibrationData> = emptyList(),
    val averageRawRssi: Float = 0f,     // Average of raw RSSI
    val averageFilteredRssi: Float = 0f,  // Average of filtered RSSI (for regression)
    val completedCalibrations: Map<Int, CalibrationResult> = emptyMap() // Store completed calibrations by distance
)

// Calibration result for a specific distance
data class CalibrationResult(
    val distance: Float,
    val averageRawRssi: Float,
    val averageFilteredRssi: Float,
    val standardDeviation: Float,
    val sampleCount: Int,
    val timestamp: Long = System.currentTimeMillis(),
    val comment: String = ""
)

// Helper functions
fun getDistanceColor(rssi: Float): Color {
    return when {
        rssi > -50 -> Color(0xFF4CAF50) // Green - good signal
        rssi > -65 -> Color(0xFFFF9800) // Orange - weak signal
        else -> Color(0xFFF44336) // Red - poor signal
    }
}

// Global logarithmic distance calculator instance
private val logarithmicCalculator = com.singleping.motoapp.distance.LogarithmicDistanceCalculator()

// Distance calculation using logarithmic regression with calibration data
fun calculateDistance(rssi: Float): Float {
    // Use logarithmic regression if calibrated, otherwise use default model
    return logarithmicCalculator.getDistance(rssi)
}

// Update the logarithmic calculator with calibration data
fun updateDistanceCalculator(calibrationData: Map<Int, CalibrationResult>) {
    logarithmicCalculator.clearCalibration()
    calibrationData.forEach { (distance, result) ->
        logarithmicCalculator.addCalibrationPoint(
            distance.toFloat(),
            result.averageFilteredRssi
        )
    }
}

// Get calculator info for debugging
fun getCalculatorModelInfo() = logarithmicCalculator.getModelInfo()

// Improved distance calculation using clustering and lookup table
private val improvedCalculator = ImprovedDistanceCalculator(
    useClusteringFilter = true,
    useLookupTable = true,
    clusteringSampleSize = 10,  // Reduced to 10 samples (1 second of data)
    clusteringVariationPercent = 10
)

fun calculateImprovedDistance(rssi: Float): ImprovedDistanceCalculator.DistanceResult? {
    return improvedCalculator.processRssiValue(rssi)
}

fun forceCalculateDistance(): ImprovedDistanceCalculator.DistanceResult? {
    return improvedCalculator.forceProcessBuffer()
}

fun getDistanceCalculatorInfo(): String {
    return "Clustering: ${improvedCalculator.isClusteringEnabled()}, " +
           "LookupTable: ${improvedCalculator.isLookupTableEnabled()}, " +
           "Buffer: ${improvedCalculator.getBufferSize()}/10"
}

fun formatConnectionTime(startTime: Long): String {
    if (startTime == 0L) return "00:00:00"
    
    val elapsed = System.currentTimeMillis() - startTime
    val seconds = (elapsed / 1000) % 60
    val minutes = (elapsed / 60000) % 60
    val hours = elapsed / 3600000
    
    return String.format("%02d:%02d:%02d", hours, minutes, seconds)
}
