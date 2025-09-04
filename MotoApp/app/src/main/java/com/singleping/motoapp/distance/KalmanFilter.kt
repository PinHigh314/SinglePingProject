package com.singleping.motoapp.distance

/**
 * 1D Kalman Filter for RSSI smoothing
 * 
 * This filter provides optimal estimation of RSSI values by:
 * - Predicting the next state based on the model
 * - Updating the prediction with new measurements
 * - Adapting the filter gain based on noise characteristics
 */
class KalmanFilter(
    private var processNoise: Float = 0.1f,     // Q: Process noise covariance
    private var measurementNoise: Float = 2.0f  // R: Measurement noise covariance
) {
    // State variables
    private var x: Float = -50f  // Initial estimate (typical RSSI value)
    private var p: Float = 1.0f  // Initial error covariance
    private var isInitialized = false
    
    /**
     * Process a new RSSI measurement through the Kalman filter
     * 
     * @param measurement The raw RSSI value
     * @return The filtered RSSI value
     */
    fun filter(measurement: Float): Float {
        // Initialize with first measurement
        if (!isInitialized) {
            x = measurement
            p = 1.0f
            isInitialized = true
            return x
        }
        
        // Prediction step
        // For RSSI, we assume a constant model (no motion model)
        val xPred = x  // State prediction
        val pPred = p + processNoise  // Error covariance prediction
        
        // Update step
        val kalmanGain = pPred / (pPred + measurementNoise)  // Kalman gain
        x = xPred + kalmanGain * (measurement - xPred)  // State update
        p = (1 - kalmanGain) * pPred  // Error covariance update
        
        return x
    }
    
    /**
     * Reset the filter to initial state
     */
    fun reset() {
        x = -50f
        p = 1.0f
        isInitialized = false
    }
    
    /**
     * Update the process noise parameter (Q)
     * Lower values = more smoothing, trusts model more
     * Higher values = less smoothing, allows faster changes
     */
    fun setProcessNoise(noise: Float) {
        processNoise = noise.coerceIn(0.001f, 10f)
    }
    
    /**
     * Update the measurement noise parameter (R)
     * Lower values = trusts measurements more
     * Higher values = trusts model more, more smoothing
     */
    fun setMeasurementNoise(noise: Float) {
        measurementNoise = noise.coerceIn(0.1f, 100f)
    }
    
    /**
     * Get current filter parameters for debugging
     */
    fun getParameters(): String {
        return "Q=$processNoise, R=$measurementNoise, K=${p / (p + measurementNoise)}"
    }
    
    /**
     * Get the current Kalman gain
     * Values close to 1 = trusting measurements
     * Values close to 0 = trusting model
     */
    fun getKalmanGain(): Float {
        return p / (p + measurementNoise)
    }
    
    /**
     * Get the current state estimate without updating
     * Used by OptimisticFilter to make accept/reject decisions
     */
    fun getCurrentEstimate(): Float = x
}
