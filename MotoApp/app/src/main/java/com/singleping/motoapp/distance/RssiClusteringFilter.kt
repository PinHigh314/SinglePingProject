package com.singleping.motoapp.distance

import kotlin.math.abs
import kotlin.math.roundToInt

/**
 * RSSI Clustering Filter based on the majority clustering algorithm
 * from the RSSI to Distance Conversion Guide.
 * 
 * This filter identifies the dominant signal path by grouping RSSI measurements
 * into clusters and selecting the majority cluster, effectively filtering out
 * multipath reflections and interference.
 */
class RssiClusteringFilter(
    private val variationPercent: Int = 10,  // 10% variation tolerance
    private val maxClusters: Int = 10
) {
    
    data class RssiCluster(
        var centerValue: Float,
        var minRange: Float,
        var maxRange: Float,
        var sampleCount: Int = 0,
        var sum: Float = 0f
    )
    
    data class ClusteringResult(
        val filteredRssi: Float,
        val confidence: Float,  // 0.0 to 1.0
        val clusterSize: Int,
        val totalClusters: Int,
        val isValid: Boolean
    )
    
    /**
     * Process a list of RSSI samples and return the filtered result
     * using majority clustering algorithm
     */
    fun processSamples(samples: List<Float>): ClusteringResult {
        if (samples.isEmpty()) {
            return ClusteringResult(
                filteredRssi = 0f,
                confidence = 0f,
                clusterSize = 0,
                totalClusters = 0,
                isValid = false
            )
        }
        
        val clusters = mutableListOf<RssiCluster>()
        
        // Process each sample
        for (sample in samples) {
            var assignedToCluster = false
            
            // Try to assign to existing cluster
            for (cluster in clusters) {
                if (isInCluster(sample, cluster)) {
                    addToCluster(cluster, sample)
                    assignedToCluster = true
                    break
                }
            }
            
            // Create new cluster if not assigned and within limit
            if (!assignedToCluster && clusters.size < maxClusters) {
                clusters.add(createCluster(sample))
            }
        }
        
        if (clusters.isEmpty()) {
            return ClusteringResult(
                filteredRssi = samples.average().toFloat(),
                confidence = 0.3f,
                clusterSize = samples.size,
                totalClusters = 1,
                isValid = true
            )
        }
        
        // Find majority cluster (largest sample count)
        val majorityCluster = clusters.maxByOrNull { it.sampleCount } ?: clusters.first()
        
        // Calculate confidence based on cluster dominance
        val confidence = calculateConfidence(
            majorityCluster.sampleCount,
            samples.size,
            clusters.size
        )
        
        return ClusteringResult(
            filteredRssi = majorityCluster.centerValue,
            confidence = confidence,
            clusterSize = majorityCluster.sampleCount,
            totalClusters = clusters.size,
            isValid = confidence >= 0.3f
        )
    }
    
    /**
     * Calculate the clustering range for a given RSSI center value
     * Handles negative RSSI values correctly for percentage-based clustering
     */
    private fun calculateClusterRange(centerRssi: Float): Pair<Float, Float> {
        // Calculate variation in dB based on absolute value
        val variationDb = (abs(centerRssi) * variationPercent) / 100f
        
        // Apply variation symmetrically around center
        val minRange = centerRssi - variationDb  // More negative (weaker signal)
        val maxRange = centerRssi + variationDb  // Less negative (stronger signal)
        
        // Ensure ranges stay within valid RSSI bounds
        return Pair(
            minRange.coerceIn(-127f, 20f),
            maxRange.coerceIn(-127f, 20f)
        )
    }
    
    /**
     * Check if an RSSI value belongs to a specific cluster
     */
    private fun isInCluster(rssiValue: Float, cluster: RssiCluster): Boolean {
        return rssiValue >= cluster.minRange && rssiValue <= cluster.maxRange
    }
    
    /**
     * Add an RSSI sample to an existing cluster and update cluster statistics
     */
    private fun addToCluster(cluster: RssiCluster, rssiValue: Float) {
        cluster.sum += rssiValue
        cluster.sampleCount++
        
        // Update cluster center using running average
        cluster.centerValue = cluster.sum / cluster.sampleCount
        
        // Recalculate cluster range based on updated center
        val (minRange, maxRange) = calculateClusterRange(cluster.centerValue)
        cluster.minRange = minRange
        cluster.maxRange = maxRange
    }
    
    /**
     * Create a new cluster with the given RSSI value as the initial center
     */
    private fun createCluster(rssiValue: Float): RssiCluster {
        val (minRange, maxRange) = calculateClusterRange(rssiValue)
        return RssiCluster(
            centerValue = rssiValue,
            minRange = minRange,
            maxRange = maxRange,
            sampleCount = 1,
            sum = rssiValue
        )
    }
    
    /**
     * Calculate confidence score based on cluster dominance
     */
    private fun calculateConfidence(
        majoritySize: Int,
        totalSamples: Int,
        clusterCount: Int
    ): Float {
        // Base confidence from cluster dominance
        var confidence = majoritySize.toFloat() / totalSamples.toFloat()
        
        // Bonus confidence for fewer clusters (cleaner environment)
        when (clusterCount) {
            1 -> confidence += 0.2f
            2 -> confidence += 0.1f
            3 -> confidence += 0.05f
        }
        
        // Cap confidence at 1.0
        return confidence.coerceIn(0f, 1f)
    }
}
