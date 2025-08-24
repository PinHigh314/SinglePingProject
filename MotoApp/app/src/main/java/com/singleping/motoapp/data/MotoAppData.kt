package com.singleping.motoapp.data

import androidx.compose.ui.graphics.Color

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
    val value: Float // in dBm
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
    val signalStrength: Float = -45f // dBm
)

// Logging data classes
data class LogData(
    val timestamp: Long = System.currentTimeMillis(),
    val rssi: Float,
    val distance: Float,
    val hostInfo: HostInfo,
    val mipeStatus: MipeStatus?
)

data class LogStats(
    val totalSamples: Int = 0,
    val samplesPerSecond: Float = 0f,
    val startTime: Long = 0L,
    val isLogging: Boolean = false
)

// Helper functions
fun getDistanceColor(rssi: Float): Color {
    return when {
        rssi > -50 -> Color(0xFF4CAF50) // Green - good signal
        rssi > -65 -> Color(0xFFFF9800) // Orange - weak signal
        else -> Color(0xFFF44336) // Red - poor signal
    }
}

fun calculateDistance(rssi: Float): Float {
    // RSSI to distance formula: Distance = 10^((Tx_Power - RSSI) / (10 * N))
    val txPower = -20f // Simulated transmit power in dBm
    val pathLossExponent = 2.0f // Path loss exponent
    
    val distance = Math.pow(10.0, ((txPower - rssi) / (10 * pathLossExponent)).toDouble()).toFloat()
    
    // Add realistic noise (±0.3m)
    val noise = ((Math.random() - 0.5) * 0.6).toFloat()
    
    return (distance + noise).coerceIn(0.1f, 50f) // Limit to reasonable range
}

fun formatConnectionTime(startTime: Long): String {
    if (startTime == 0L) return "00:00:00"
    
    val elapsed = System.currentTimeMillis() - startTime
    val seconds = (elapsed / 1000) % 60
    val minutes = (elapsed / 60000) % 60
    val hours = elapsed / 3600000
    
    return String.format("%02d:%02d:%02d", hours, minutes, seconds)
}
