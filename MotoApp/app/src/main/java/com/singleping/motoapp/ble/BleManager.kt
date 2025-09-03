package com.singleping.motoapp.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothManager
import android.content.Context
import android.util.Log
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import no.nordicsemi.android.ble.BleManager
import no.nordicsemi.android.ble.data.Data
import java.util.UUID
import com.singleping.motoapp.data.MipeStatus

/**
 * BLE Manager for connecting to the Host device (MIPE_HOST_A1B2)
 * Handles real BLE connections and data streaming
 */
class HostBleManager(context: Context) : BleManager(context) {
    
    companion object {
        private const val TAG = "HostBleManager"
        
        // Host device name to scan for
        const val HOST_DEVICE_NAME = "MIPE_HOST_A1B2"
        
        // TMT1 Service and Characteristic UUIDs (matching host firmware)
        val TMT1_SERVICE_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef0")
        val RSSI_DATA_CHAR_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef1")
        val CONTROL_CHAR_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef2")
        val STATUS_CHAR_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef3")
        val MIPE_STATUS_CHAR_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef4")
        val LOG_DATA_CHAR_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef5")
        
        // Control commands
        const val CMD_START_STREAM: Byte = 0x01
        const val CMD_STOP_STREAM: Byte = 0x02
        const val CMD_GET_STATUS: Byte = 0x03
        const val CMD_MIPE_SYNC: Byte = 0x04
    }
    
    // Connection state
    private val _connectionState = MutableStateFlow(false)
    val connectionState: StateFlow<Boolean> = _connectionState.asStateFlow()
    
    // RSSI data callback - now includes battery values
    var onRssiDataReceived: ((rssi: Int, hostBatteryMv: Int, mipeBatteryMv: Int) -> Unit)? = null
    var onMipeStatusReceived: ((status: MipeStatus) -> Unit)? = null
    var onLogDataReceived: ((log: String) -> Unit)? = null
    
    // GATT characteristics
    private var rssiDataCharacteristic: BluetoothGattCharacteristic? = null
    private var controlCharacteristic: BluetoothGattCharacteristic? = null
    private var statusCharacteristic: BluetoothGattCharacteristic? = null
    private var mipeStatusCharacteristic: BluetoothGattCharacteristic? = null
    private var logDataCharacteristic: BluetoothGattCharacteristic? = null
    
    override fun getGattCallback(): BleManagerGattCallback {
        return object : BleManagerGattCallback() {
            
            @Deprecated("since 2.7.0")
            override fun isRequiredServiceSupported(gatt: BluetoothGatt): Boolean {
                val service = gatt.getService(TMT1_SERVICE_UUID)
                if (service != null) {
                    rssiDataCharacteristic = service.getCharacteristic(RSSI_DATA_CHAR_UUID)
                    controlCharacteristic = service.getCharacteristic(CONTROL_CHAR_UUID)
                    statusCharacteristic = service.getCharacteristic(STATUS_CHAR_UUID)
                    mipeStatusCharacteristic = service.getCharacteristic(MIPE_STATUS_CHAR_UUID)
                    logDataCharacteristic = service.getCharacteristic(LOG_DATA_CHAR_UUID)
                    
                    return rssiDataCharacteristic != null && 
                           controlCharacteristic != null && 
                           statusCharacteristic != null &&
                           mipeStatusCharacteristic != null &&
                           logDataCharacteristic != null
                }
                return false
            }
            
            @Deprecated("since 2.7.0")
            override fun onServicesInvalidated() {
                rssiDataCharacteristic = null
                controlCharacteristic = null
                statusCharacteristic = null
                mipeStatusCharacteristic = null
                logDataCharacteristic = null
            }
            
            @Deprecated("since 2.7.0")
            override fun initialize() {
                // Enable notifications for RSSI data
                rssiDataCharacteristic?.let { characteristic ->
                    setNotificationCallback(characteristic).with { _, data ->
                        handleRssiData(data)
                    }
                    enableNotifications(characteristic).enqueue()
                }

                mipeStatusCharacteristic?.let { characteristic ->
                    setNotificationCallback(characteristic).with { _, data ->
                        handleMipeStatusData(data)
                    }
                    enableNotifications(characteristic).enqueue()
                }

                logDataCharacteristic?.let { characteristic ->
                    setNotificationCallback(characteristic).with { _, data ->
                        handleLogData(data)
                    }
                    enableNotifications(characteristic).enqueue()
                }
                
                // Connection is now ready
                _connectionState.value = true
                Log.i(TAG, "BLE connection initialized successfully")
            }
            
            @Deprecated("since 2.7.0")
            override fun onDeviceDisconnected() {
                _connectionState.value = false
                Log.i(TAG, "Device disconnected")
            }
        }
    }
    
    private fun handleRssiData(data: Data) {
        // Check for new 5-byte format first, fall back to old 4-byte format
        if (data.size() >= 5) {
            // NEW FORMAT: 5 bytes
            // Bytes 0-1: Host battery (uint16_t, little-endian)
            // Bytes 2-3: Mipe battery (uint16_t, little-endian)
            // Byte 4: RSSI value (int8_t)
            
            val hostBatteryMv = data.getIntValue(Data.FORMAT_UINT16_LE, 0) ?: 0
            val mipeBatteryMv = data.getIntValue(Data.FORMAT_UINT16_LE, 2) ?: 0
            val rssiValue = data.getByte(4)?.toInt() ?: 0
            
            Log.d(TAG, "Received RSSI bundle: RSSI=$rssiValue dBm, Host=$hostBatteryMv mV, Mipe=$mipeBatteryMv mV")
            onRssiDataReceived?.invoke(rssiValue, hostBatteryMv, mipeBatteryMv)
            
        } else if (data.size() >= 4) {
            // OLD FORMAT: 4 bytes (fallback for compatibility)
            // Byte 0: RSSI value (signed)
            // Bytes 1-3: Timestamp (24-bit)
            
            val rssiValue = data.getByte(0)?.toInt() ?: 0
            
            Log.d(TAG, "Received RSSI data (old format): $rssiValue dBm")
            // Use 0 for battery values when not available
            onRssiDataReceived?.invoke(rssiValue, 0, 0)
        } else {
            Log.w(TAG, "Invalid RSSI data size: ${data.size()} bytes")
        }
    }

    private fun handleLogData(data: Data) {
        val logString = data.getStringValue(0)
        if (logString != null) {
            Log.d(TAG, "Received log data: $logString")
            onLogDataReceived?.invoke(logString)
        }
    }

    private fun handleMipeStatusData(data: Data) {
        if (data.size() >= 12) {
            val connectionStateValue = data.getByte(0)?.toInt() ?: 0
            val rssi = data.getByte(1)?.toInt() ?: 0
            val deviceAddressBytes = data.value?.sliceArray(2..7)
            val deviceAddress = deviceAddressBytes?.joinToString(":") { "%02X".format(it) }
            val connectionDuration = data.getIntValue(Data.FORMAT_UINT32_LE, 8) ?: 0

            val connectionState = when (connectionStateValue) {
                0 -> "Idle"
                1 -> "Scanning"
                2 -> "Connected"
                3 -> "Connected" // Also connected
                4 -> "Disconnected"
                else -> "Unknown"
            }

            // Parse battery voltage if available (extended data format)
            var batteryVoltage: Float? = null
            if (data.size() >= 16) {
                // Parse float bytes directly (host sends raw float bytes)
                val batteryBytes = data.value?.sliceArray(12..15)
                if (batteryBytes != null && batteryBytes.size == 4) {
                    batteryVoltage = java.nio.ByteBuffer
                        .wrap(batteryBytes)
                        .order(java.nio.ByteOrder.LITTLE_ENDIAN)
                        .float
                }
            }

            val status = MipeStatus(
                connectionState = connectionState,
                rssi = rssi,
                deviceName = null, // Not available in the current data packet
                deviceAddress = deviceAddress,
                connectionDuration = connectionDuration.toLong(),
                lastSeen = System.currentTimeMillis(),
                batteryVoltage = batteryVoltage
            )
            onMipeStatusReceived?.invoke(status)
        }
    }
    
    /**
     * Start data streaming from the host
     */
    suspend fun startDataStream() {
        controlCharacteristic?.let { characteristic ->
            val data = Data(byteArrayOf(CMD_START_STREAM))
            writeCharacteristic(
                characteristic,
                data,
                BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
            ).enqueue()
            Log.i(TAG, "Start stream command sent")
        }
    }
    
    /**
     * Stop data streaming from the host
     */
    suspend fun stopDataStream() {
        controlCharacteristic?.let { characteristic ->
            val data = Data(byteArrayOf(CMD_STOP_STREAM))
            writeCharacteristic(
                characteristic,
                data,
                BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
            ).enqueue()
            Log.i(TAG, "Stop stream command sent")
        }
    }
    
    /**
     * Read status from the host
     */
    suspend fun readStatus(): ByteArray? {
        // TODO: Implement status reading when needed
        // For now, return null as this is not critical for basic BLE functionality
        return null
    }

    /**
     * Sync with Mipe device - Phase 1 implementation
     * Sends a sync command to the host to connect to Mipe for battery reading
     */
    suspend fun syncWithMipe() {
        controlCharacteristic?.let { characteristic ->
            val data = Data(byteArrayOf(CMD_MIPE_SYNC))
            writeCharacteristic(
                characteristic,
                data,
                BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
            ).enqueue()
            Log.i(TAG, "Mipe sync command sent to host")
        } ?: run {
            Log.w(TAG, "Cannot send sync command - control characteristic not available")
            throw IllegalStateException("Control characteristic not available")
        }
    }
    
    /**
     * Check if Bluetooth is enabled
     */
    @SuppressLint("MissingPermission")
    fun isBluetoothEnabled(context: Context): Boolean {
        val bluetoothManager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        return bluetoothManager.adapter?.isEnabled == true
    }
}
