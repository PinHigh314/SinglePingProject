package com.singleping.motoapp.export

import android.content.ContentValues
import android.content.Context
import android.os.Build
import android.os.Environment
import android.provider.MediaStore
import android.util.Log
import com.singleping.motoapp.data.CalibrationState
import com.singleping.motoapp.data.LogData
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File
import java.io.FileOutputStream
import java.text.SimpleDateFormat
import java.util.*

class DataExporter(private val context: Context) {

    companion object {
        private const val TAG = "DataExporter"
    }

    suspend fun exportCalibrationData(calibrationState: CalibrationState) {
        withContext(Dispatchers.IO) {
            try {
                val jsonContent = generateJsonContent(calibrationState)
                val timestamp = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())
                val fileName = "Calibration_${calibrationState.selectedDistance}m_$timestamp.json"

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    // Use MediaStore for Android 10+
                    val contentValues = ContentValues().apply {
                        put(MediaStore.Downloads.DISPLAY_NAME, fileName)
                        put(MediaStore.Downloads.MIME_TYPE, "application/json")
                        put(MediaStore.Downloads.IS_PENDING, 1)
                    }
                    
                    val resolver = context.contentResolver
                    val uri = resolver.insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, contentValues)
                    
                    uri?.let {
                        resolver.openOutputStream(it)?.use { outputStream ->
                            outputStream.write(jsonContent.toByteArray())
                        }
                        
                        contentValues.clear()
                        contentValues.put(MediaStore.Downloads.IS_PENDING, 0)
                        resolver.update(uri, contentValues, null, null)
                        
                        Log.i(TAG, "Calibration file saved to Downloads: $fileName")
                    }
                } else {
                    // Use direct file access for Android 9 and below
                    val downloadsDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
                    val file = File(downloadsDir, fileName)
                    FileOutputStream(file).use {
                        it.write(jsonContent.toByteArray())
                    }
                    Log.i(TAG, "Calibration file saved to: ${file.absolutePath}")
                }
            } catch (e: Exception) {
                Log.e(TAG, "Failed to save calibration file", e)
                throw e
            }
        }
    }

    private fun generateJsonContent(calibrationState: CalibrationState): String {
        val gson = com.google.gson.GsonBuilder().setPrettyPrinting().create()
        return gson.toJson(calibrationState)
    }

    suspend fun exportLogData(logData: List<LogData>) {
        withContext(Dispatchers.IO) {
            try {
                val timestamp = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())
                
                // Create CSV content with detailed logging information
                val csvContent = buildString {
                    // Header row with all available fields
                    appendLine("Timestamp,Time,RSSI (dBm),Filtered RSSI (dBm),Distance (m),Host Battery (mV),Host Battery (V),Mipe Battery (mV),Mipe Battery (%),Host Device,Signal Strength,Mipe Connection State,Mipe Device Name,Mipe Device Address,Mipe RSSI,Connection Duration (s)")
                    
                    // Data rows
                    val dateFormat = SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.US)
                    logData.forEach { log ->
                        val timeStr = dateFormat.format(Date(log.timestamp))
                        val mipeBatteryPercent = if (log.mipeBatteryMv > 0) {
                            calculateBatteryPercentage(log.mipeBatteryMv)
                        } else {
                            "N/A"
                        }
                        
                        // Extract Mipe status fields safely
                        val mipeConnectionState = log.mipeStatus?.connectionState ?: "Unknown"
                        val mipeDeviceName = log.mipeStatus?.deviceName ?: "N/A"
                        val mipeDeviceAddress = log.mipeStatus?.deviceAddress ?: "N/A"
                        val mipeRssi = log.mipeStatus?.rssi ?: 0
                        val connectionDuration = log.mipeStatus?.connectionDuration?.let { it / 1000 } ?: 0
                        
                        append("${log.timestamp},")
                        append("$timeStr,")
                        append("${log.rssi},")
                        append("${String.format("%.2f", log.filteredRssi)},")
                        append("${String.format("%.2f", log.distance)},")
                        append("${log.hostBatteryMv},")
                        append("${String.format("%.2f", log.hostInfo.batteryVoltage)},")
                        append("${log.mipeBatteryMv},")
                        append("$mipeBatteryPercent,")
                        append("${log.hostInfo.deviceName},")
                        append("${log.hostInfo.signalStrength},")
                        append("$mipeConnectionState,")
                        append("$mipeDeviceName,")
                        append("$mipeDeviceAddress,")
                        append("$mipeRssi,")
                        appendLine("$connectionDuration")
                    }
                }

                // Save CSV file using MediaStore for Android 10+ or direct file access for older versions
                val csvFileName = "MotoApp_Log_$timestamp.csv"
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    // Use MediaStore for Android 10+
                    val contentValues = ContentValues().apply {
                        put(MediaStore.Downloads.DISPLAY_NAME, csvFileName)
                        put(MediaStore.Downloads.MIME_TYPE, "text/csv")
                        put(MediaStore.Downloads.IS_PENDING, 1)
                    }
                    
                    val resolver = context.contentResolver
                    val uri = resolver.insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, contentValues)
                    
                    uri?.let {
                        resolver.openOutputStream(it)?.use { outputStream ->
                            outputStream.write(csvContent.toByteArray())
                        }
                        
                        contentValues.clear()
                        contentValues.put(MediaStore.Downloads.IS_PENDING, 0)
                        resolver.update(uri, contentValues, null, null)
                        
                        Log.i(TAG, "CSV file saved to Downloads: $csvFileName")
                    }
                } else {
                    // Use direct file access for Android 9 and below
                    val downloadsDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
                    val file = File(downloadsDir, csvFileName)
                    FileOutputStream(file).use {
                        it.write(csvContent.toByteArray())
                    }
                    Log.i(TAG, "CSV file saved to: ${file.absolutePath}")
                }
                
                // Also create a JSON version for detailed analysis
                
                // Create simplified log data with formatted timestamps
                val simplifiedData = logData.map { log ->
                    val dateFormat = SimpleDateFormat("dd.MM.yy_HH.mm.ss", Locale.US)
                    val formattedTimestamp = dateFormat.format(Date(log.timestamp))
                    
                    mapOf(
                        "Mipe rssi" to log.rssi,
                        "filteredRssi" to String.format("%.2f", log.filteredRssi),
                        "distance" to log.distance,
                        "hostBatteryMv" to log.hostBatteryMv,
                        "mipeBatteryMv" to log.mipeBatteryMv,
                        "timestamp" to formattedTimestamp
                    )
                }
                
                val gson = com.google.gson.GsonBuilder()
                    .setPrettyPrinting()
                    .create()
                
                // Load calibration reference data
                val calibrationData = loadCalibrationReference()
                
                val jsonContent = gson.toJson(mapOf(
                    "exportTime" to timestamp,
                    "totalSamples" to logData.size,
                    "duration" to if (logData.isNotEmpty()) {
                        val durationMs = logData.last().timestamp - logData.first().timestamp
                        "${durationMs / 1000} seconds"
                    } else "0 seconds",
                    "calibrationReference" to calibrationData,
                    "data" to simplifiedData
                ))
                
                // Save JSON file using MediaStore for Android 10+ or direct file access for older versions
                val jsonFileName = "MotoApp_Log_${timestamp}.json"
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    // Use MediaStore for Android 10+
                    val contentValues = ContentValues().apply {
                        put(MediaStore.Downloads.DISPLAY_NAME, jsonFileName)
                        put(MediaStore.Downloads.MIME_TYPE, "application/json")
                        put(MediaStore.Downloads.IS_PENDING, 1)
                    }
                    
                    val resolver = context.contentResolver
                    val uri = resolver.insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, contentValues)
                    
                    uri?.let {
                        resolver.openOutputStream(it)?.use { outputStream ->
                            outputStream.write(jsonContent.toByteArray())
                        }
                        
                        contentValues.clear()
                        contentValues.put(MediaStore.Downloads.IS_PENDING, 0)
                        resolver.update(uri, contentValues, null, null)
                        
                        Log.i(TAG, "JSON file saved to Downloads: $jsonFileName")
                    }
                } else {
                    // Use direct file access for Android 9 and below
                    val downloadsDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
                    val jsonFile = File(downloadsDir, jsonFileName)
                    FileOutputStream(jsonFile).use {
                        it.write(jsonContent.toByteArray())
                    }
                    Log.i(TAG, "JSON file saved to: ${jsonFile.absolutePath}")
                }
                
            } catch (e: Exception) {
                Log.e(TAG, "Failed to export log data", e)
                throw e
            }
        }
    }
    
    private fun calculateBatteryPercentage(batteryMv: Int): String {
        // Battery voltage thresholds for CR2032
        // 3.0V (3000mV) = 100%
        // 2.0V (2000mV) = 0%
        val percentage = when {
            batteryMv >= 3000 -> 100
            batteryMv <= 2000 -> 0
            else -> ((batteryMv - 2000) * 100) / 1000
        }
        return "$percentage%"
    }
    
    private fun loadCalibrationReference(): Map<String, Any> {
        // Default calibration values for 8 distances
        // These should be loaded from persistent storage or calibration file
        return mapOf(
            "1m" to mapOf("rssi" to -45.0, "filteredRssi" to -45.0),
            "2m" to mapOf("rssi" to -52.0, "filteredRssi" to -52.0),
            "3m" to mapOf("rssi" to -57.0, "filteredRssi" to -57.0),
            "4m" to mapOf("rssi" to -60.0, "filteredRssi" to -60.0),
            "5m" to mapOf("rssi" to -63.0, "filteredRssi" to -63.0),
            "6m" to mapOf("rssi" to -65.0, "filteredRssi" to -65.0),
            "7m" to mapOf("rssi" to -67.0, "filteredRssi" to -67.0),
            "8m" to mapOf("rssi" to -69.0, "filteredRssi" to -69.0)
        )
    }
}
