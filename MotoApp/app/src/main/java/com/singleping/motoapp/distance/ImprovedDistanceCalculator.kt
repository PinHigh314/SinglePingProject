package com.singleping.motoapp.distance

import android.util.Log

/**
 * Improved Distance Calculator that combines RSSI clustering with logarithmic regression
 * for accurate distance measurements.
 */
class ImprovedDistanceCalculator(
    private val useClusteringFilter: Boolean = true,
    private val useLookupTable: Boolean = false,  // Deprecated - now uses logarithmic regression
    private val clusteringSampleSize: Int = 10,  // Reduced from 20 to 10 for 1-second updates
    private val clusteringVariationPercent: Int = 10
) {
    companion object {
        private const val TAG = "ImprovedDistanceCalc"
    }
    
    private val clusteringFilter = RssiClusteringFilter(
        variationPercent = clusteringVariationPercent
    )
    private val logarithmicCalculator = LogarithmicDistanceCalculator()
    
    // Buffer for collecting RSSI samples for clustering
    private val rssiBuffer = mutableListOf<Float>()
    private var lastClusteringResult: RssiClusteringFilter.ClusteringResult? = null
    
    data class DistanceResult(
        val distance: Float,
        val filteredRssi: Float,
        val confidence: Float,
        val method: String,
        val clusterInfo: ClusterInfo? = null
    )
    
    data class ClusterInfo(
        val clusterCount: Int,
        val majorityClusterSize: Int,
        val totalSamples: Int
    )
    
    /**
     * Process a new RSSI value and calculate distance
     * This method handles buffering for clustering if enabled
     */
    fun processRssiValue(rssi: Float): DistanceResult? {
        return if (useClusteringFilter) {
            // Add to buffer for clustering
            rssiBuffer.add(rssi)
            
            // Process when buffer reaches desired size
            if (rssiBuffer.size >= clusteringSampleSize) {
                val result = processBufferedSamples()
                // Keep last few samples for continuity
                val keepSamples = clusteringSampleSize / 4
                rssiBuffer.clear()
                rssiBuffer.addAll(rssiBuffer.takeLast(keepSamples))
                result
            } else {
                // Return null while collecting samples
                null
            }
        } else {
            // Direct calculation without clustering
            calculateDistanceDirectly(rssi)
        }
    }
    
    /**
     * Force processing of current buffer even if not full
     * Useful for getting immediate results
     */
    fun forceProcessBuffer(): DistanceResult? {
        return if (rssiBuffer.isNotEmpty()) {
            processBufferedSamples()
        } else {
            null
        }
    }
    
    /**
     * Process buffered samples with clustering
     */
    private fun processBufferedSamples(): DistanceResult {
        val clusteringResult = clusteringFilter.processSamples(rssiBuffer)
        lastClusteringResult = clusteringResult
        
        if (!clusteringResult.isValid) {
            Log.w(TAG, "Clustering result invalid, using average")
            val avgRssi = rssiBuffer.average().toFloat()
            return calculateDistanceDirectly(avgRssi)
        }
        
        // Use logarithmic regression for distance calculation
        val distance = logarithmicCalculator.getDistance(clusteringResult.filteredRssi)
        
        // Calculate confidence based on RSSI strength
        val distanceConfidence = getDistanceConfidence(clusteringResult.filteredRssi)
        val combinedConfidence = (clusteringResult.confidence * 0.7f + distanceConfidence * 0.3f)
        
        Log.d(TAG, "Clustered RSSI: ${clusteringResult.filteredRssi} dBm, " +
                "Distance: $distance m, " +
                "Clusters: ${clusteringResult.totalClusters}, " +
                "Confidence: ${(combinedConfidence * 100).toInt()}%")
        
        return DistanceResult(
            distance = distance,
            filteredRssi = clusteringResult.filteredRssi,
            confidence = combinedConfidence,
            method = "Clustered+LogarithmicRegression",
            clusterInfo = ClusterInfo(
                clusterCount = clusteringResult.totalClusters,
                majorityClusterSize = clusteringResult.clusterSize,
                totalSamples = rssiBuffer.size
            )
        )
    }
    
    /**
     * Calculate distance directly without clustering
     */
    private fun calculateDistanceDirectly(rssi: Float): DistanceResult {
        // Use logarithmic regression for distance calculation
        val distance = logarithmicCalculator.getDistance(rssi)
        
        val confidence = getDistanceConfidence(rssi)
        
        return DistanceResult(
            distance = distance,
            filteredRssi = rssi,
            confidence = confidence,
            method = "Direct+LogarithmicRegression",
            clusterInfo = null
        )
    }
    
    /**
     * Get confidence factor based on RSSI value
     * Higher confidence for RSSI values within optimal range
     */
    private fun getDistanceConfidence(rssi: Float): Float {
        return when {
            rssi > -30 -> 0.95f  // Very close, high confidence
            rssi > -50 -> 0.85f  // Good signal range
            rssi > -70 -> 0.70f  // Medium range
            rssi > -85 -> 0.50f  // Weak signal
            else -> 0.30f        // Very weak signal
        }
    }
    
    /**
     * Theoretical distance calculation as fallback
     */
    private fun calculateTheoreticalDistance(rssi: Float): Float {
        val txPower = -20f
        val pathLossExponent = 2.2f // Outdoor environment
        val distance = Math.pow(10.0, ((txPower - rssi) / (10 * pathLossExponent)).toDouble()).toFloat()
        return distance.coerceIn(0.1f, 300f)
    }
    
    /**
     * Get the last clustering result for debugging/display
     */
    fun getLastClusteringResult(): RssiClusteringFilter.ClusteringResult? {
        return lastClusteringResult
    }
    
    /**
     * Clear the RSSI buffer
     */
    fun clearBuffer() {
        rssiBuffer.clear()
        lastClusteringResult = null
    }
    
    /**
     * Get current buffer size
     */
    fun getBufferSize(): Int {
        return rssiBuffer.size
    }
    
    /**
     * Check if clustering is enabled
     */
    fun isClusteringEnabled(): Boolean {
        return useClusteringFilter
    }
    
    /**
     * Check if lookup table is enabled (deprecated - always returns false)
     */
    fun isLookupTableEnabled(): Boolean {
        return false  // Lookup table is deprecated, using logarithmic regression
    }
    
    /**
     * Update the logarithmic calculator with calibration data
     */
    fun updateCalibration(calibrationPoints: Map<Float, Float>) {
        logarithmicCalculator.clearCalibration()
        calibrationPoints.forEach { (distance, rssi) ->
            logarithmicCalculator.addCalibrationPoint(distance, rssi)
        }
    }
}
