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
import kotlin.math.sin
import kotlin.random.Random

/**
 * Enhanced ViewModel that supports both real BLE and simulation modes
 */
class MotoAppBleViewModel(application: Application) : AndroidViewModel(application) {
    
    companion object {
        private const val TAG = "MotoAppBleViewModel"
    }
    
    // BLE components
    private val bleManager = HostBleManager(application)
    private val bleScanner = BleScanner(application)
    
    // Operation mode
    private val _useRealBle = MutableStateFlow(true) // Default to real BLE
    val useRealBle: StateFlow<Boolean> = _useRealBle.asStateFlow()
    
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
    
    private val _errorMessage = MutableStateFlow<String?>(null)
    val errorMessage: StateFlow<String?> = _errorMessage.asStateFlow()
    
    // Coroutine jobs
    private var connectionJob: Job? = null
    private var streamingJob: Job? = null
    private var connectionTimeJob: Job? = null
    
    // Simulation parameters
    private var baseRssi = -55f
    private var simulationTime = 0L
    
    init {
        // Set up BLE callbacks
        setupBleCallbacks()
        
        // Monitor BLE connection state
        viewModelScope.launch {
            bleManager.connectionState.collect { isConnected ->
                if (_useRealBle.value) {
                    handleBleConnectionChange(isConnected)
                }
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
    }
    
    /**
     * Toggle between real BLE and simulation mode
     */
    fun toggleMode() {
        viewModelScope.launch {
            if (_connectionState.value.isConnected) {
                disconnect()
            }
            _useRealBle.value = !_useRealBle.value
            _errorMessage.value = if (_useRealBle.value) {
                "Switched to Real BLE mode"
            } else {
                "Switched to Simulation mode"
            }
            Log.i(TAG, "Mode changed to: ${if (_useRealBle.value) "Real BLE" else "Simulation"}")
        }
    }
    
    /**
     * Toggle connection (works for both real and simulated)
     */
    fun toggleConnection() {
        if (_connectionState.value.isConnected) {
            disconnect()
        } else {
            connect()
        }
    }
    
    private fun connect() {
        if (_useRealBle.value) {
            connectRealBle()
        } else {
            connectSimulated()
        }
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
                        Log.i(TAG, "Device found: ${device.name} (${device.address})")
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
            _errorMessage.value = "Connecting to ${device.name}..."
            
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
    
    private fun connectSimulated() {
        connectionJob?.cancel()
        connectionJob = viewModelScope.launch {
            // Update state to connecting
            _connectionState.value = _connectionState.value.copy(
                isConnecting = true,
                isConnected = false
            )
            _errorMessage.value = "Simulating connection..."
            
            // Simulate discovery time (2-3 seconds)
            delay(Random.nextLong(2000, 3000))
            
            // Simulate connection establishment (1-2 seconds)
            delay(Random.nextLong(1000, 2000))
            
            // Connection successful
            val connectionTime = System.currentTimeMillis()
            _connectionState.value = _connectionState.value.copy(
                isConnecting = false,
                isConnected = true,
                connectionTime = connectionTime
            )
            _errorMessage.value = "Simulated connection established"
            
            // Start updating connection time
            startConnectionTimer()
        }
    }
    
    private fun disconnect() {
        // Cancel all jobs
        connectionJob?.cancel()
        streamingJob?.cancel()
        connectionTimeJob?.cancel()
        
        if (_useRealBle.value) {
            // Disconnect BLE
            bleScanner.stopScan()
            bleManager.disconnect().enqueue()
        }
        
        // Reset states
        _connectionState.value = ConnectionState()
        _streamState.value = StreamState()
        _rssiHistory.value = emptyList()
        _distanceData.value = DistanceData()
        simulationTime = 0L
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
        
        if (_useRealBle.value) {
            startRealDataStream()
        } else {
            startSimulatedDataStream()
        }
    }
    
    private fun startRealDataStream() {
        viewModelScope.launch {
            try {
                // Send start stream command to host
                bleManager.startDataStream()
                _streamState.value = _streamState.value.copy(isStreaming = true)
                _errorMessage.value = "Data streaming started"
                
            } catch (e: Exception) {
                Log.e(TAG, "Failed to start data stream", e)
                _errorMessage.value = "Failed to start streaming: ${e.message}"
            }
        }
    }
    
    private fun startSimulatedDataStream() {
        streamingJob?.cancel()
        streamingJob = viewModelScope.launch {
            _streamState.value = _streamState.value.copy(isStreaming = true)
            _errorMessage.value = "Simulated streaming started"
            
            while (_streamState.value.isStreaming) {
                // Generate simulated RSSI data
                val rssiValue = generateSimulatedRssi()
                val timestamp = System.currentTimeMillis()
                
                handleRssiData(rssiValue, timestamp)
                
                // Wait for next update (100ms)
                delay(_streamState.value.updateRate.toLong())
                simulationTime += _streamState.value.updateRate
            }
        }
    }
    
    private fun stopDataStream() {
        streamingJob?.cancel()
        
        if (_useRealBle.value) {
            viewModelScope.launch {
                try {
                    bleManager.stopDataStream()
                } catch (e: Exception) {
                    Log.e(TAG, "Failed to stop data stream", e)
                }
            }
        }
        
        _streamState.value = _streamState.value.copy(isStreaming = false)
        _errorMessage.value = "Data streaming stopped"
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
    }
    
    private fun generateSimulatedRssi(): Float {
        // Create realistic RSSI variations
        // Base RSSI slowly varies to simulate movement
        baseRssi += (Random.nextFloat() - 0.5f) * 0.5f
        baseRssi = baseRssi.coerceIn(-75f, -40f)
        
        // Add periodic variation (simulate walking pattern)
        val periodicVariation = sin(simulationTime / 2000.0) * 5.0
        
        // Add random noise (±2 dBm)
        val noise = (Random.nextFloat() - 0.5f) * 4f
        
        val rssi = (baseRssi + periodicVariation + noise).toFloat()
        return rssi.coerceIn(-80f, -30f)
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
