package com.singleping.motoapp.storage

import android.content.Context
import android.content.SharedPreferences
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import com.singleping.motoapp.data.CalibrationResult

/**
 * Manages persistent storage of calibration data using SharedPreferences
 */
class CalibrationStorage(context: Context) {
    
    companion object {
        private const val PREFS_NAME = "motoapp_calibration_prefs"
        private const val KEY_CALIBRATION_DATA = "calibration_data"
        private const val KEY_LAST_CALIBRATION_TIME = "last_calibration_time"
    }
    
    private val prefs: SharedPreferences = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    private val gson = Gson()
    
    /**
     * Save calibration data to persistent storage
     */
    fun saveCalibrations(calibrations: Map<Int, CalibrationResult>) {
        val json = gson.toJson(calibrations)
        prefs.edit().apply {
            putString(KEY_CALIBRATION_DATA, json)
            putLong(KEY_LAST_CALIBRATION_TIME, System.currentTimeMillis())
            apply()
        }
    }
    
    /**
     * Load calibration data from persistent storage
     */
    fun loadCalibrations(): Map<Int, CalibrationResult> {
        val json = prefs.getString(KEY_CALIBRATION_DATA, null) ?: return emptyMap()
        
        return try {
            val type = object : TypeToken<Map<Int, CalibrationResult>>() {}.type
            gson.fromJson(json, type) ?: emptyMap()
        } catch (e: Exception) {
            // If there's an error parsing, return empty map
            emptyMap()
        }
    }
    
    /**
     * Clear all saved calibration data
     */
    fun clearCalibrations() {
        prefs.edit().apply {
            remove(KEY_CALIBRATION_DATA)
            remove(KEY_LAST_CALIBRATION_TIME)
            apply()
        }
    }
    
    /**
     * Get the timestamp of the last calibration
     */
    fun getLastCalibrationTime(): Long {
        return prefs.getLong(KEY_LAST_CALIBRATION_TIME, 0L)
    }
    
    /**
     * Check if there are saved calibrations
     */
    fun hasCalibrations(): Boolean {
        return prefs.contains(KEY_CALIBRATION_DATA)
    }
    
    /**
     * Get the number of saved calibration points
     */
    fun getCalibrationCount(): Int {
        val calibrations = loadCalibrations()
        return calibrations.size
    }
}
