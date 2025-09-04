package com.singleping.motoapp.distance

import kotlin.math.ln
import kotlin.math.pow
import kotlin.math.sqrt

/**
 * Logarithmic regression-based distance calculator
 * Uses calibration points to create a path loss model for distance estimation
 */
class LogarithmicDistanceCalculator {
    
    // Calibration points: distance (meters) -> average filtered RSSI (dBm)
    private val calibrationPoints = mutableMapOf<Float, Float>()
    
    // Path loss model parameters
    // Distance = 10^((A - RSSI) / (10 * n))
    // Where A = RSSI at 1 meter, n = path loss exponent
    private var referenceRssi: Float = -40f  // A: RSSI at 1 meter (will be calculated)
    private var pathLossExponent: Float = 2.0f  // n: typically 2-4 (will be calculated)
    
    // Statistics for model quality
    private var rSquared: Float = 0f  // Coefficient of determination
    private var rmse: Float = 0f  // Root mean square error
    
    /**
     * Add or update a calibration point
     * @param distance The known distance in meters
     * @param avgFilteredRssi The average filtered RSSI at this distance
     */
    fun addCalibrationPoint(distance: Float, avgFilteredRssi: Float) {
        calibrationPoints[distance] = avgFilteredRssi
        if (calibrationPoints.size >= 2) {
            calculateRegression()
        }
    }
    
    /**
     * Remove a calibration point
     */
    fun removeCalibrationPoint(distance: Float) {
        calibrationPoints.remove(distance)
        if (calibrationPoints.size >= 2) {
            calculateRegression()
        }
    }
    
    /**
     * Clear all calibration points
     */
    fun clearCalibration() {
        calibrationPoints.clear()
        referenceRssi = -40f
        pathLossExponent = 2.0f
        rSquared = 0f
        rmse = 0f
    }
    
    /**
     * Calculate logarithmic regression parameters using least squares method
     * Model: RSSI = A - 10 * n * log10(distance)
     * Rearranged: distance = 10^((A - RSSI) / (10 * n))
     */
    private fun calculateRegression() {
        if (calibrationPoints.size < 2) return
        
        // Convert to logarithmic space for linear regression
        // Y = RSSI, X = log10(distance)
        val points = calibrationPoints.map { (distance, rssi) ->
            Pair(kotlin.math.log10(distance), rssi)
        }
        
        // Calculate means
        val meanX = points.map { it.first }.average()
        val meanY = points.map { it.second }.average()
        
        // Calculate slope (b) and intercept (a)
        // Y = a + b*X where b = -10*n and a = A
        var numerator = 0.0
        var denominator = 0.0
        
        points.forEach { (x, y) ->
            numerator += (x - meanX) * (y - meanY)
            denominator += (x - meanX) * (x - meanX)
        }
        
        val slope = if (denominator != 0.0) numerator / denominator else -20.0
        val intercept = meanY - slope * meanX
        
        // Extract model parameters
        pathLossExponent = (-slope / 10.0).toFloat()
        referenceRssi = intercept.toFloat()
        
        // Ensure reasonable bounds
        pathLossExponent = pathLossExponent.coerceIn(1.5f, 4.5f)
        
        // Calculate R-squared for model quality
        calculateModelQuality()
    }
    
    /**
     * Calculate R-squared and RMSE for model quality assessment
     */
    private fun calculateModelQuality() {
        if (calibrationPoints.isEmpty()) {
            rSquared = 0f
            rmse = 0f
            return
        }
        
        val actualValues = mutableListOf<Float>()
        val predictedValues = mutableListOf<Float>()
        
        calibrationPoints.forEach { (distance, actualRssi) ->
            val predictedRssi = predictRssi(distance)
            actualValues.add(actualRssi)
            predictedValues.add(predictedRssi)
        }
        
        val meanActual = actualValues.average()
        
        // Calculate R-squared
        var ssRes = 0.0  // Residual sum of squares
        var ssTot = 0.0  // Total sum of squares
        var sumSquaredError = 0.0
        
        actualValues.forEachIndexed { index, actual ->
            val predicted = predictedValues[index]
            val residual = actual - predicted
            ssRes += residual * residual
            ssTot += (actual - meanActual) * (actual - meanActual)
            sumSquaredError += residual * residual
        }
        
        rSquared = if (ssTot != 0.0) {
            (1 - ssRes / ssTot).toFloat().coerceIn(0f, 1f)
        } else {
            0f
        }
        
        // Calculate RMSE
        rmse = sqrt(sumSquaredError / actualValues.size).toFloat()
    }
    
    /**
     * Predict RSSI for a given distance using the model
     */
    private fun predictRssi(distance: Float): Float {
        return referenceRssi - 10 * pathLossExponent * kotlin.math.log10(distance)
    }
    
    /**
     * Calculate distance from filtered RSSI using the logarithmic model
     * @param filteredRssi The filtered RSSI value in dBm
     * @return Estimated distance in meters
     */
    fun getDistance(filteredRssi: Float): Float {
        // Ensure we have a valid model
        if (calibrationPoints.size < 2) {
            // Fallback to simple path loss model with default parameters
            return 10f.pow((referenceRssi - filteredRssi) / (10f * pathLossExponent))
        }
        
        // Calculate distance using the calibrated model
        val distance = 10f.pow((referenceRssi - filteredRssi) / (10f * pathLossExponent))
        
        // Apply reasonable bounds
        return distance.coerceIn(0.1f, 100f)
    }
    
    /**
     * Get model parameters for debugging/display
     */
    fun getModelInfo(): ModelInfo {
        return ModelInfo(
            referenceRssi = referenceRssi,
            pathLossExponent = pathLossExponent,
            rSquared = rSquared,
            rmse = rmse,
            calibrationPointCount = calibrationPoints.size
        )
    }
    
    /**
     * Get all calibration points
     */
    fun getCalibrationPoints(): Map<Float, Float> = calibrationPoints.toMap()
    
    /**
     * Load calibration from a map
     */
    fun loadCalibration(points: Map<Float, Float>) {
        calibrationPoints.clear()
        calibrationPoints.putAll(points)
        if (calibrationPoints.size >= 2) {
            calculateRegression()
        }
    }
    
    /**
     * Check if calculator has enough calibration data
     */
    fun isCalibrated(): Boolean = calibrationPoints.size >= 2
    
    data class ModelInfo(
        val referenceRssi: Float,
        val pathLossExponent: Float,
        val rSquared: Float,
        val rmse: Float,
        val calibrationPointCount: Int
    )
}
