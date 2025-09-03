package com.singleping.motoapp.distance

import kotlin.math.abs
import kotlin.math.sign

/**
 * Velocity-based smoothing filter for distance measurements.
 * Ensures distance changes are physically realistic based on human movement speeds.
 */
class VelocitySmoothing {
    
    companion object {
        // Movement speed constants (in meters per second)
        const val WALKING_SPEED = 1.4f      // Normal walking ~5 km/h
        const val BRISK_WALK_SPEED = 2.0f   // Brisk walking ~7.2 km/h
        const val RUNNING_SPEED = 3.5f      // Running ~12.6 km/h
        const val MAX_SUDDEN_SPEED = 2.5f   // Maximum sudden movement for safety
        
        // Confidence thresholds
        const val HIGH_CONFIDENCE_THRESHOLD = 0.75f
        const val MEDIUM_CONFIDENCE_THRESHOLD = 0.5f
    }
    
    // Tracking variables
    private var lastDistance: Float? = null
    private var lastUpdateTime: Long = 0L
    private var smoothedDistance: Float = 0f
    private var targetDistance: Float = 0f
    private var isTracking: Boolean = false
    
    // Configuration
    private var maxVelocity: Float = WALKING_SPEED
    private var smoothingFactor: Float = 0.15f // Balanced smoothing - 15% per update
    private val maxJumpDistance: Float = 5.0f // Increased to allow legitimate changes
    
    data class SmoothedResult(
        val distance: Float,
        val isSmoothed: Boolean,
        val velocityLimited: Boolean,
        val currentVelocity: Float
    )
    
    /**
     * Process a new distance measurement with velocity-based smoothing
     */
    fun processDistance(
        newDistance: Float,
        confidence: Float,
        timestamp: Long = System.currentTimeMillis()
    ): SmoothedResult {
        
        // Initialize on first measurement
        if (lastDistance == null || lastUpdateTime == 0L) {
            lastDistance = newDistance
            lastUpdateTime = timestamp
            smoothedDistance = newDistance
            targetDistance = newDistance
            isTracking = true
            
            return SmoothedResult(
                distance = newDistance,
                isSmoothed = false,
                velocityLimited = false,
                currentVelocity = 0f
            )
        }
        
        // Calculate time delta in seconds
        val timeDelta = (timestamp - lastUpdateTime) / 1000f
        if (timeDelta <= 0) {
            return SmoothedResult(
                distance = smoothedDistance,
                isSmoothed = false,
                velocityLimited = false,
                currentVelocity = 0f
            )
        }
        
        // Calculate proposed distance change
        val distanceChange = newDistance - lastDistance!!
        val absChange = abs(distanceChange)
        val velocity = absChange / timeDelta
        
        // Hard limit: only reject truly impossible changes
        if (absChange > maxJumpDistance && velocity > RUNNING_SPEED * 2) {
            // Only reject if both distance and velocity are unrealistic
            return SmoothedResult(
                distance = smoothedDistance,
                isSmoothed = true,
                velocityLimited = true,
                currentVelocity = velocity
            )
        }
        
        // Determine max allowed velocity based on confidence
        val effectiveMaxVelocity = when {
            confidence > HIGH_CONFIDENCE_THRESHOLD -> maxVelocity
            confidence > MEDIUM_CONFIDENCE_THRESHOLD -> maxVelocity * 1.2f  // Reduced from 1.5f
            else -> maxVelocity * 1.5f  // Even low confidence shouldn't allow huge jumps
        }
        
        var velocityLimited = false
        var resultDistance = newDistance
        
        // Apply velocity constraint
        if (velocity > effectiveMaxVelocity) {
            // Limit the distance change based on maximum velocity
            val maxDistanceChange = effectiveMaxVelocity * timeDelta
            val limitedChange = maxDistanceChange * sign(distanceChange)
            resultDistance = lastDistance!! + limitedChange
            velocityLimited = true
            
            // Update target for gradual catching up
            targetDistance = newDistance
        } else {
            // Accept the measurement
            targetDistance = newDistance
        }
        
        // Apply adaptive smoothing based on change magnitude
        val adaptiveSmoothingFactor = when {
            absChange > 3.0f -> 0.1f   // Moderate for very large changes (10%)
            absChange > 1.5f -> 0.15f  // Balanced for large changes (15%)
            absChange > 0.5f -> 0.2f   // Responsive for medium changes (20%)
            absChange > 0.2f -> 0.3f   // Quick for small changes (30%)
            else -> 0.4f                // Very responsive for tiny changes (40%)
        }
        
        // Apply smoothing to gradually approach target
        smoothedDistance = smoothedDistance + (resultDistance - smoothedDistance) * adaptiveSmoothingFactor
        
        // Update tracking variables
        lastDistance = smoothedDistance
        lastUpdateTime = timestamp
        
        return SmoothedResult(
            distance = smoothedDistance,
            isSmoothed = true,
            velocityLimited = velocityLimited,
            currentVelocity = velocity
        )
    }
    
    /**
     * Set the maximum allowed velocity (for different movement modes)
     */
    fun setMaxVelocity(velocity: Float) {
        maxVelocity = velocity.coerceIn(WALKING_SPEED, RUNNING_SPEED)
    }
    
    /**
     * Set movement mode
     */
    fun setMovementMode(mode: MovementMode) {
        maxVelocity = when (mode) {
            MovementMode.WALKING -> WALKING_SPEED
            MovementMode.BRISK_WALKING -> BRISK_WALK_SPEED
            MovementMode.RUNNING -> RUNNING_SPEED
        }
    }
    
    /**
     * Reset the smoothing filter
     */
    fun reset() {
        lastDistance = null
        lastUpdateTime = 0L
        smoothedDistance = 0f
        targetDistance = 0f
        isTracking = false
    }
    
    /**
     * Get current smoothing status
     */
    fun getStatus(): String {
        return "Tracking: $isTracking, Max Velocity: ${maxVelocity}m/s, " +
               "Current: ${String.format("%.2f", smoothedDistance)}m, " +
               "Target: ${String.format("%.2f", targetDistance)}m"
    }
    
    enum class MovementMode {
        WALKING,
        BRISK_WALKING,
        RUNNING
    }
}
