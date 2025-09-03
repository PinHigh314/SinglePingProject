package com.singleping.motoapp.export

import android.content.Context
import android.os.Environment
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

                val downloadsDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
                val file = File(downloadsDir, fileName)

                FileOutputStream(file).use {
                    it.write(jsonContent.toByteArray())
                }

                Log.i(TAG, "File saved to: ${file.absolutePath}")
                withContext(Dispatchers.Main) {
                    // You can use this callback to show a toast or a notification
                    // onExportComplete?.invoke(true, "File saved to Downloads folder")
                }
            } catch (e: Exception) {
                Log.e(TAG, "Failed to save file", e)
                withContext(Dispatchers.Main) {
                    // onExportComplete?.invoke(false, "Failed to save file: ${e.message}")
                }
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
                val fileName = "MotoApp_Log_$timestamp.csv"

                val downloadsDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
                val file = File(downloadsDir, fileName)

                // Create CSV content with detailed logging information
                val csvContent = buildString {
                    // Header row with all available fields
                    appendLine("Timestamp,Time,RSSI (dBm),Distance (m),Host Battery (mV),Host Battery (V),Mipe Battery (mV),Mipe Battery (%),Host Device,Signal Strength,Mipe Connection State,Mipe Device Name,Mipe Device Address,Mipe RSSI,Connection Duration (s)")
                    
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

                FileOutputStream(file).use {
                    it.write(csvContent.toByteArray())
                }

                Log.i(TAG, "Log data exported to: ${file.absolutePath}")
                
                // Also create a JSON version for detailed analysis
                val jsonFileName = "MotoApp_Log_${timestamp}.json"
                val jsonFile = File(downloadsDir, jsonFileName)
                
                val gson = com.google.gson.GsonBuilder()
                    .setPrettyPrinting()
                    .create()
                
                val jsonContent = gson.toJson(mapOf(
                    "exportTime" to timestamp,
                    "totalSamples" to logData.size,
                    "duration" to if (logData.isNotEmpty()) {
                        val durationMs = logData.last().timestamp - logData.first().timestamp
                        "${durationMs / 1000} seconds"
                    } else "0 seconds",
                    "data" to logData
                ))
                
                FileOutputStream(jsonFile).use {
                    it.write(jsonContent.toByteArray())
                }
                
                Log.i(TAG, "JSON log data exported to: ${jsonFile.absolutePath}")
                
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
}
