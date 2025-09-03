package com.singleping.motoapp.distance

import kotlin.math.abs
import kotlin.math.roundToInt

/**
 * Distance Lookup Table based on calibrated RSSI to distance measurements.
 * Uses interpolation for values between table entries.
 */
class DistanceLookupTable {
    
    // Lookup table from calibrated data (RSSI in dBm -> Distance in meters)
    private val lookupTable = mapOf(
        -20 to 0.14f,
        -21 to 0.15f,
        -22 to 0.16f,
        -23 to 0.18f,
        -24 to 0.20f,
        -25 to 0.22f,
        -26 to 0.24f,
        -27 to 0.26f,
        -28 to 0.29f,
        -29 to 0.32f,
        -30 to 0.35f,
        -31 to 0.39f,
        -32 to 0.43f,
        -33 to 0.47f,
        -34 to 0.51f,
        -35 to 0.57f,
        -36 to 0.62f,
        -37 to 0.68f,
        -38 to 0.75f,
        -39 to 0.83f,
        -40 to 0.91f,
        -41 to 1.0f,
        -42 to 1.1f,
        -43 to 1.21f,
        -44 to 1.33f,
        -45 to 1.46f,
        -46 to 1.61f,
        -47 to 1.77f,
        -48 to 1.94f,
        -49 to 2.14f,
        -50 to 2.35f,
        -51 to 2.58f,
        -52 to 2.84f,
        -53 to 3.12f,
        -54 to 3.43f,
        -55 to 3.78f,
        -56 to 4.15f,
        -57 to 4.57f,
        -58 to 5.02f,
        -59 to 5.52f,
        -60 to 6.07f,
        -61 to 6.68f,
        -62 to 7.34f,
        -63 to 8.07f,
        -64 to 8.88f,
        -65 to 9.76f,
        -66 to 10.73f,
        -67 to 11.8f,
        -68 to 12.98f,
        -69 to 14.27f,
        -70 to 15.69f,
        -71 to 17.25f,
        -72 to 18.97f,
        -73 to 20.86f,
        -74 to 22.94f,
        -75 to 25.23f,
        -76 to 27.74f,
        -77 to 30.5f,
        -78 to 33.54f,
        -79 to 36.88f,
        -80 to 40.56f,
        -81 to 44.6f,
        -82 to 49.04f,
        -83 to 53.93f,
        -84 to 59.3f,
        -85 to 65.21f,
        -86 to 71.7f,
        -87 to 78.85f,
        -88 to 86.7f,
        -89 to 95.34f,
        -90 to 104.84f,
        -91 to 115.28f,
        -92 to 126.77f,
        -93 to 139.4f,
        -94 to 153.28f,
        -95 to 168.55f,
        -96 to 185.35f,
        -97 to 203.81f,
        -98 to 224.11f,
        -99 to 246.44f,
        -100 to 270.99f
    )
    
    /**
     * Get distance for a given RSSI value using lookup table with interpolation
     */
    fun getDistance(rssi: Float): Float {
        val rssiInt = rssi.roundToInt()
        
        // Direct lookup if exact match exists
        lookupTable[rssiInt]?.let { return it }
        
        // Handle out of bounds cases
        if (rssiInt > -20) {
            // Stronger than minimum RSSI, return minimum distance
            return lookupTable[-20] ?: 0.14f
        }
        if (rssiInt < -100) {
            // Weaker than maximum RSSI, return maximum distance
            return lookupTable[-100] ?: 270.99f
        }
        
        // Interpolation for values between table entries
        return interpolateDistance(rssi)
    }
    
    /**
     * Linear interpolation between two table entries
     */
    private fun interpolateDistance(rssi: Float): Float {
        // Properly floor the RSSI value for negative numbers
        val lowerRssi = kotlin.math.floor(rssi).toInt()
        val upperRssi = lowerRssi + 1
        
        val lowerDistance = lookupTable[lowerRssi]
        val upperDistance = lookupTable[upperRssi]
        
        // If either bound is missing, try to find nearest values
        if (lowerDistance == null || upperDistance == null) {
            return findNearestDistance(rssi)
        }
        
        // Linear interpolation
        val fraction = rssi - lowerRssi
        return lowerDistance + (upperDistance - lowerDistance) * fraction
    }
    
    /**
     * Find the nearest available distance value when interpolation is not possible
     */
    private fun findNearestDistance(rssi: Float): Float {
        val rssiInt = rssi.roundToInt()
        
        // Search for nearest available value
        for (offset in 1..5) {
            lookupTable[rssiInt - offset]?.let { return it }
            lookupTable[rssiInt + offset]?.let { return it }
        }
        
        // Fallback to theoretical calculation if no nearby values found
        return calculateTheoreticalDistance(rssi)
    }
    
    /**
     * Fallback theoretical distance calculation
     */
    private fun calculateTheoreticalDistance(rssi: Float): Float {
        val txPower = -20f
        val pathLossExponent = 2.2f // Outdoor environment
        val distance = Math.pow(10.0, ((txPower - rssi) / (10 * pathLossExponent)).toDouble()).toFloat()
        return distance.coerceIn(0.1f, 300f)
    }
    
    /**
     * Get the valid RSSI range for the lookup table
     */
    fun getValidRssiRange(): Pair<Int, Int> {
        return Pair(-100, -20)
    }
    
    /**
     * Get confidence factor based on RSSI value
     * Higher confidence for RSSI values within optimal range
     */
    fun getDistanceConfidence(rssi: Float): Float {
        return when {
            rssi > -30 -> 0.95f  // Very close, high confidence
            rssi > -50 -> 0.85f  // Good signal range
            rssi > -70 -> 0.70f  // Medium range
            rssi > -85 -> 0.50f  // Weak signal
            else -> 0.30f        // Very weak signal
        }
    }
}
