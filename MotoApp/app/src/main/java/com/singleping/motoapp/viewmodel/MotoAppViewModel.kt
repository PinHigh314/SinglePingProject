package com.singleping.motoapp.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.singleping.motoapp.data.*
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlin.math.sin
import kotlin.random.Random

class MotoAppViewModel : ViewModel() {
    
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
    
    // Coroutine jobs
    private var connectionJob: Job? = null
    private var streamingJob: Job? = null
    private var connectionTimeJob: Job? = null
    
    // Simulation parameters
    private var baseRssi = -55f
    private var simulationTime = 0L
    
    fun toggleConnection() {
        if (_connectionState.value.isConnected) {
            disconnect()
        } else {
            connect()
        }
    }
    
    private fun connect() {
        connectionJob?.cancel()
        connectionJob = viewModelScope.launch {
            // Update state to connecting
            _connectionState.value = _connectionState.value.copy(
                isConnecting = true,
                isConnected = false
            )
            
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
            
            // Start updating connection time
            startConnectionTimer()
        }
    }
    
    private fun disconnect() {
        // Cancel all jobs
        connectionJob?.cancel()
        streamingJob?.cancel()
        connectionTimeJob?.cancel()
        
        // Reset states
        _connectionState.value = ConnectionState()
        _streamState.value = StreamState()
        _rssiHistory.value = emptyList()
        _distanceData.value = DistanceData()
        simulationTime = 0L
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
        
        streamingJob?.cancel()
        streamingJob = viewModelScope.launch {
            _streamState.value = _streamState.value.copy(isStreaming = true)
            
            while (_streamState.value.isStreaming) {
                // Generate simulated RSSI data
                val rssiValue = generateSimulatedRssi()
                val timestamp = System.currentTimeMillis()
                
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
                
                // Wait for next update (100ms)
                delay(_streamState.value.updateRate.toLong())
                simulationTime += _streamState.value.updateRate
            }
        }
    }
    
    private fun stopDataStream() {
        streamingJob?.cancel()
        _streamState.value = _streamState.value.copy(isStreaming = false)
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
    
    override fun onCleared() {
        super.onCleared()
        connectionJob?.cancel()
        streamingJob?.cancel()
        connectionTimeJob?.cancel()
    }
}
