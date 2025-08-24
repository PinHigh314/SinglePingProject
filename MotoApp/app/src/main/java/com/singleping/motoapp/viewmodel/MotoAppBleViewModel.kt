package com.singleping.motoapp.viewmodel

import android.app.Application
import android.bluetooth.BluetoothDevice
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.singleping.motoapp.ble.BleScanner
import com.singleping.motoapp.ble.HostBleManager
import com.singleping.motoapp.data.*
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

/**
 * ViewModel for managing BLE connections and data streaming
 */
class MotoAppBleViewModel(application: Application) : AndroidViewModel(application) {
    
    companion object {
        private const val TAG = "MotoAppBleViewModel"
    }
    
    // BLE components
    private val bleManager = HostBleManager(application)
    private val bleScanner = BleScanner(application)
    
    // State flows for UI
    private val _connectionState = MutableStateFlow(ConnectionState())
    val connectionState: StateFlow<ConnectionState> = _connectionState.asStateFlow()
    
    private val _streamState = MutableStateFlow(StreamState())
    val streamState: StateFlow<StreamState> = _streamState.asStateFlow()
    
    private val _rssiHistory = MutableStateFlow<List<RssiData>>(emptyList())
    val rssiHistory: StateFlow<List<RssiData>> = _rssiHistory.asStateFlow()
    
    private val _distanceData = MutableStateFlow(DistanceData())
    val distanceData: StateFlow<DistanceData> = _distanceData.asStateFlow()
    
    private val _hostInfo = MutableStateFlow(HostInfo())
    val hostInfo: StateFlow<HostInfo> = _hostInfo.asStateFlow()

    private val _mipeStatus = MutableStateFlow<MipeStatus?>(null)
    val mipeStatus: StateFlow<MipeStatus?> = _mipeStatus.asStateFlow()
    
    private val _logHistory = MutableStateFlow<List<String>>(emptyList())
    val logHistory: StateFlow<List<String>> = _logHistory.asStateFlow()

    // Logging state
    private val _loggingData = MutableStateFlow<List<LogData>>(emptyList())
    val loggingData: StateFlow<List<LogData>> = _loggingData.asStateFlow()

    private val _logStats = MutableStateFlow(LogStats())
    val logStats: StateFlow<LogStats> = _logStats.asStateFlow()
    
    private val _errorMessage = MutableStateFlow<String?>(null)
    val errorMessage: StateFlow<String?> = _errorMessage.asStateFlow()
    
    // Coroutine jobs
    private var connectionJob: Job? = null
    private var streamingJob: Job? = null
    private var connectionTimeJob: Job? = null
    private var logStatsJob: Job? = null
    
    init {
        // Set up BLE callbacks
        setupBleCallbacks()
        
        // Monitor BLE connection state
        viewModelScope.launch {
            bleManager.connectionState.collect { isConnected ->
                handleBleConnectionChange(isConnected)
            }
        }
    }
    
    private fun setupBleCallbacks() {
        // Set up RSSI data callback
        bleManager.onRssiDataReceived = { rssi, timestamp ->
            viewModelScope.launch {
                handleRealRssiData(rssi, timestamp)
            }
        }

        // Set up Mipe status callback
        bleManager.onMipeStatusReceived = { status ->
            viewModelScope.launch {
                _mipeStatus.value = status
            }
        }

        bleManager.onLogDataReceived = { log ->
            viewModelScope.launch {
                val updatedLogs = (_logHistory.value + log).takeLast(100)
                _logHistory.value = updatedLogs
            }
        }
    }
    
    
    /**
     * Toggle connection
     */
    fun toggleConnection() {
        if (_connectionState.value.isConnected) {
            disconnect()
        } else {
            connect()
        }
    }
    
    private fun connect() {
        connectRealBle()
    }
    
    private fun connectRealBle() {
        connectionJob?.cancel()
        connectionJob = viewModelScope.launch {
            try {
                // Update state to connecting
                _connectionState.value = _connectionState.value.copy(
                    isConnecting = true,
                    isConnected = false
                )
                _errorMessage.value = "Scanning for host device..."
                
                // Check if Bluetooth is enabled
                if (!bleManager.isBluetoothEnabled(getApplication())) {
                    _errorMessage.value = "Bluetooth is disabled. Please enable Bluetooth."
                    _connectionState.value = _connectionState.value.copy(isConnecting = false)
                    return@launch
                }
                
                // Scan for the host device
                Log.i(TAG, "Starting BLE scan for host device")
                bleScanner.scanForHost(
                    onDeviceFound = { device ->
                        Log.i(TAG, "Device found: ${device.address}")
                        viewModelScope.launch {
                            connectToDevice(device)
                        }
                    },
                    onScanFailed = { error ->
                        Log.e(TAG, "Scan failed: $error")
                        viewModelScope.launch {
                            _errorMessage.value = error
                            _connectionState.value = _connectionState.value.copy(isConnecting = false)
                        }
                    }
                )
                
            } catch (e: Exception) {
                Log.e(TAG, "Connection error", e)
                _errorMessage.value = "Connection failed: ${e.message}"
                _connectionState.value = _connectionState.value.copy(isConnecting = false)
            }
        }
    }
    
    private suspend fun connectToDevice(device: BluetoothDevice) {
        try {
            _errorMessage.value = "Connecting to device..."
            
            // Connect using BLE Manager
            bleManager.connect(device)
                .retry(3, 100)
                .useAutoConnect(false)
                .timeout(10000)
                .enqueue()
            
            // Connection state will be updated via the connectionState flow
            
        } catch (e: Exception) {
            Log.e(TAG, "Failed to connect to device", e)
            _errorMessage.value = "Failed to connect: ${e.message}"
            _connectionState.value = _connectionState.value.copy(isConnecting = false)
        }
    }
    
    private fun handleBleConnectionChange(isConnected: Boolean) {
        if (isConnected) {
            // Connection successful
            val connectionTime = System.currentTimeMillis()
            _connectionState.value = _connectionState.value.copy(
                isConnecting = false,
                isConnected = true,
                connectionTime = connectionTime
            )
            _errorMessage.value = "Connected to host device"
            
            // Start updating connection time
            startConnectionTimer()
            
        } else {
            // Disconnected
            _connectionState.value = ConnectionState()
            _streamState.value = StreamState()
            _errorMessage.value = "Disconnected from host"
            connectionTimeJob?.cancel()
        }
    }
    
    
    private fun disconnect() {
        // Cancel all jobs
        connectionJob?.cancel()
        streamingJob?.cancel()
        connectionTimeJob?.cancel()
        
        // Disconnect BLE
        bleScanner.stopScan()
        bleManager.disconnect().enqueue()
        
        // Reset states
        _connectionState.value = ConnectionState()
        _streamState.value = StreamState()
        _rssiHistory.value = emptyList()
        _distanceData.value = DistanceData()
        _errorMessage.value = null
    }
    
    private fun startConnectionTimer() {
        connectionTimeJob?.cancel()
        connectionTimeJob = viewModelScope.launch {
            while (_connectionState.value.isConnected) {
                delay(1000) // Update every second
                // Trigger recomposition by updating a dummy value
                _connectionState.value = _connectionState.value.copy()
            }
        }
    }
    
    fun toggleDataStream() {
        if (_streamState.value.isStreaming) {
            stopDataStream()
        } else {
            startDataStream()
        }
    }
    
    private fun startDataStream() {
        if (!_connectionState.value.isConnected) return
        startRealDataStream()
    }
    
    private fun startRealDataStream() {
        viewModelScope.launch {
            try {
                // Send start stream command to host
                bleManager.startDataStream()
                _streamState.value = _streamState.value.copy(isStreaming = true)
                _errorMessage.value = "Data streaming started"
                
                // Start logging when streaming begins
                startLogging()
                
            } catch (e: Exception) {
                Log.e(TAG, "Failed to start data stream", e)
                _errorMessage.value = "Failed to start streaming: ${e.message}"
            }
        }
    }
    
    
    private fun stopDataStream() {
        streamingJob?.cancel()
        
        viewModelScope.launch {
            try {
                bleManager.stopDataStream()
            } catch (e: Exception) {
                Log.e(TAG, "Failed to stop data stream", e)
            }
        }
        
        _streamState.value = _streamState.value.copy(isStreaming = false)
        _errorMessage.value = "Data streaming stopped"
        
        // Stop logging when streaming ends
        stopLogging()
    }
    
    private fun handleRealRssiData(rssi: Int, timestamp: Long) {
        handleRssiData(rssi.toFloat(), timestamp)
    }
    
    private fun handleRssiData(rssiValue: Float, timestamp: Long) {
        // Add to history (keep last 300 values - 30 seconds at 100ms)
        val newRssiData = RssiData(timestamp, rssiValue)
        val updatedHistory = (_rssiHistory.value + newRssiData).takeLast(300)
        _rssiHistory.value = updatedHistory
        
        // Update packet count
        _streamState.value = _streamState.value.copy(
            packetsReceived = _streamState.value.packetsReceived + 1
        )
        
        // Calculate distance
        val distance = calculateDistance(rssiValue)
        updateDistanceData(distance)
        
        // Update host info with current RSSI
        _hostInfo.value = _hostInfo.value.copy(
            signalStrength = rssiValue
        )
        
        // Log the data if streaming is active
        if (_streamState.value.isStreaming) {
            logData(rssiValue, distance)
        }
    }
    
    private fun logData(rssi: Float, distance: Float) {
        val logEntry = LogData(
            rssi = rssi,
            distance = distance,
            hostInfo = _hostInfo.value,
            mipeStatus = _mipeStatus.value
        )
        
        // Add to logging data (keep last 1000 samples)
        val updatedLogs = (_loggingData.value + logEntry).takeLast(1000)
        _loggingData.value = updatedLogs
        
        // Update log statistics
        updateLogStats()
    }
    
    private fun updateLogStats() {
        val currentStats = _logStats.value
        val totalSamples = _loggingData.value.size
        val currentTime = System.currentTimeMillis()
        
        val samplesPerSecond = if (currentStats.startTime > 0 && totalSamples > 0) {
            val elapsedSeconds = (currentTime - currentStats.startTime) / 1000f
            totalSamples / elapsedSeconds
        } else 0f
        
        _logStats.value = currentStats.copy(
            totalSamples = totalSamples,
            samplesPerSecond = samplesPerSecond,
            isLogging = _streamState.value.isStreaming
        )
    }
    
    fun startLogging() {
        _logStats.value = LogStats(startTime = System.currentTimeMillis())
    }
    
    fun stopLogging() {
        // Preserve the total samples count when stopping logging
        val currentStats = _logStats.value
        _logStats.value = currentStats.copy(
            isLogging = false,
            samplesPerSecond = 0f
        )
    }
    
    fun clearLogData() {
        _loggingData.value = emptyList()
        _logStats.value = LogStats()
    }
    
    fun exportLogData(): List<LogData> {
        return _loggingData.value
    }
    
    
    private fun updateDistanceData(newDistance: Float) {
        val currentData = _distanceData.value
        val allDistances = _rssiHistory.value.map { calculateDistance(it.value) }
        
        val avgDistance = if (allDistances.isNotEmpty()) {
            allDistances.average().toFloat()
        } else newDistance
        
        val minDist = if (currentData.minDistance == Float.MAX_VALUE) {
            newDistance
        } else {
            minOf(currentData.minDistance, newDistance)
        }
        
        val maxDist = maxOf(currentData.maxDistance, newDistance)
        
        _distanceData.value = DistanceData(
            currentDistance = newDistance,
            averageDistance = avgDistance,
            minDistance = minDist,
            maxDistance = maxDist,
            confidence = 0.5f,
            lastUpdated = System.currentTimeMillis(),
            sampleCount = currentData.sampleCount + 1
        )
    }
    
    fun clearError() {
        _errorMessage.value = null
    }
    
    override fun onCleared() {
        super.onCleared()
        disconnect()
    }
}
