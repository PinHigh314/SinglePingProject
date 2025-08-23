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
        
        // Control commands
        const val CMD_START_STREAM: Byte = 0x01
        const val CMD_STOP_STREAM: Byte = 0x02
        const val CMD_GET_STATUS: Byte = 0x03
    }
    
    // Connection state
    private val _connectionState = MutableStateFlow(false)
    val connectionState: StateFlow<Boolean> = _connectionState.asStateFlow()
    
    // RSSI data callback
    var onRssiDataReceived: ((rssi: Int, timestamp: Long) -> Unit)? = null
    
    // GATT characteristics
    private var rssiDataCharacteristic: BluetoothGattCharacteristic? = null
    private var controlCharacteristic: BluetoothGattCharacteristic? = null
    private var statusCharacteristic: BluetoothGattCharacteristic? = null
    
    override fun getGattCallback(): BleManagerGattCallback {
        return object : BleManagerGattCallback() {
            
            override fun isRequiredServiceSupported(gatt: BluetoothGatt): Boolean {
                val service = gatt.getService(TMT1_SERVICE_UUID)
                if (service != null) {
                    rssiDataCharacteristic = service.getCharacteristic(RSSI_DATA_CHAR_UUID)
                    controlCharacteristic = service.getCharacteristic(CONTROL_CHAR_UUID)
                    statusCharacteristic = service.getCharacteristic(STATUS_CHAR_UUID)
                    
                    return rssiDataCharacteristic != null && 
                           controlCharacteristic != null && 
                           statusCharacteristic != null
                }
                return false
            }
            
            override fun onServicesInvalidated() {
                rssiDataCharacteristic = null
                controlCharacteristic = null
                statusCharacteristic = null
            }
            
            override fun initialize() {
                // Enable notifications for RSSI data
                rssiDataCharacteristic?.let { characteristic ->
                    setNotificationCallback(characteristic).with { _, data ->
                        handleRssiData(data)
                    }
                    enableNotifications(characteristic).enqueue()
                }
                
                // Connection is now ready
                _connectionState.value = true
                Log.i(TAG, "BLE connection initialized successfully")
            }
            
            override fun onDeviceDisconnected() {
                _connectionState.value = false
                Log.i(TAG, "Device disconnected")
            }
        }
    }
    
    private fun handleRssiData(data: Data) {
        if (data.size() >= 4) {
            val rssiValue = data.getByte(0)?.toInt() ?: 0
            // Get 24-bit timestamp as Long
            val timestampInt = data.getIntValue(Data.FORMAT_UINT24_LE, 1) ?: 0
            val timestamp = timestampInt.toLong()
            
            Log.d(TAG, "Received RSSI data: $rssiValue dBm, timestamp: $timestamp")
            onRssiDataReceived?.invoke(rssiValue, timestamp)
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
     * Check if Bluetooth is enabled
     */
    @SuppressLint("MissingPermission")
    fun isBluetoothEnabled(context: Context): Boolean {
        val bluetoothManager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        return bluetoothManager.adapter?.isEnabled == true
    }
}
