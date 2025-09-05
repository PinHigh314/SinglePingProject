package com.singleping.motoapp.distance

/**
 * Optimistic Filter for RSSI pre-processing
 * 
 * This filter applies an asymmetrical rule to RSSI values:
 * - Eagerly accepts stronger signals (better line of sight)
 * - Skeptically evaluates weaker signals (potential reflections)
 * 
 * Applied before Kalman filtering to improve distance measurement stability
 */
class OptimisticFilter(
    private val rejectionThreshold: Float = 5.0f,  // Default 5 dBm threshold for rejection
    private val maxConsecutiveRejections: Int = 5  // After this many rejections, accept the value
) {
    private var lastAcceptedValue: Float? = null
    private var consecutiveRejections = 0
    
    /**
     * Filter an RSSI value based on optimistic rules
     * 
     * @param rawRssi The new raw RSSI measurement
     * @param currentEstimate The current filtered estimate (from Kalman filter)
     * @return The accepted RSSI value, or null if rejected
     */
    fun filter(rawRssi: Float, currentEstimate: Float): Float? {
        // Initialize on first reading
        if (lastAcceptedValue == null) {
            lastAcceptedValue = rawRssi
            return rawRssi
        }
        
        return when {
            // Signal got stronger - good news! Accept immediately
            // This allows quick response to improved line of sight
            rawRssi > currentEstimate -> {
                consecutiveRejections = 0  // Reset counter on stronger signal
                lastAcceptedValue = rawRssi
                rawRssi
            }
            
            // Signal got weaker - be skeptical
            else -> {
                val delta = currentEstimate - rawRssi
                
                if (delta > rejectionThreshold) {
                    // Signal is significantly weaker - check if we should still accept
                    consecutiveRejections++
                    
                    if (consecutiveRejections >= maxConsecutiveRejections) {
                        // Too many rejections - the weak signal is persistent, not a glitch
                        // Accept it to avoid getting stuck
                        consecutiveRejections = 0
                        lastAcceptedValue = rawRssi
                        rawRssi
                    } else {
                        // Reject this reading as it's likely interference
                        null
                    }
                } else {
                    // Reset counter when accepting a value
                    consecutiveRejections = 0
                    // Signal is only slightly weaker - normal variation
                    // Accept this reading as it's within expected noise
                    lastAcceptedValue = rawRssi
                    rawRssi
                }
            }
        }
    }
    
    /**
     * Reset the filter to initial state
     */
    fun reset() {
        lastAcceptedValue = null
        consecutiveRejections = 0
    }
    
    /**
     * Get the rejection threshold
     */
    fun getRejectionThreshold(): Float = rejectionThreshold
    
    /**
     * Get the current rejection threshold
     */
    fun getRejectionThreshold(): Float = rejectionThreshold
    
    /**
     * Reset the filter state
     */
    fun reset() {
        lastAcceptedValue = null
    }
}
