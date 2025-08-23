package com.singleping.motoapp.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothDevice
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.ParcelUuid
import android.util.Log
import no.nordicsemi.android.support.v18.scanner.BluetoothLeScannerCompat
import no.nordicsemi.android.support.v18.scanner.ScanCallback as NordicScanCallback
import no.nordicsemi.android.support.v18.scanner.ScanFilter as NordicScanFilter
import no.nordicsemi.android.support.v18.scanner.ScanResult as NordicScanResult
import no.nordicsemi.android.support.v18.scanner.ScanSettings as NordicScanSettings

/**
 * BLE Scanner for finding the Host device
 */
class BleScanner(private val context: Context) {
    
    companion object {
        private const val TAG = "BleScanner"
        private const val SCAN_TIMEOUT_MS = 10000L // 10 seconds
    }
    
    private val scanner = BluetoothLeScannerCompat.getScanner()
    private var scanCallback: NordicScanCallback? = null
    private var isScanning = false
    
    /**
     * Scan for the Host device
     * @param onDeviceFound Callback when the host device is found
     * @param onScanFailed Callback when scan fails
     */
    @SuppressLint("MissingPermission")
    fun scanForHost(
        onDeviceFound: (BluetoothDevice) -> Unit,
        onScanFailed: (String) -> Unit
    ) {
        if (isScanning) {
            Log.w(TAG, "Already scanning")
            return
        }
        
        Log.i(TAG, "Starting scan for ${HostBleManager.HOST_DEVICE_NAME}")
        
        // Don't filter by UUID - scan for all devices and filter by name
        // The Host might not advertise the service UUID
        val scanFilters = emptyList<NordicScanFilter>()
        
        // Scan settings for low latency
        val scanSettings = NordicScanSettings.Builder()
            .setScanMode(NordicScanSettings.SCAN_MODE_LOW_LATENCY)
            .setReportDelay(0)
            .setUseHardwareBatchingIfSupported(false)
            .build()
        
        // Create scan callback
        scanCallback = object : NordicScanCallback() {
            override fun onScanResult(callbackType: Int, result: NordicScanResult) {
                val device = result.device
                val deviceName = device.name
                val rssi = result.rssi
                
                // Log all devices for debugging
                if (deviceName != null) {
                    Log.d(TAG, "Found device: $deviceName (${device.address}) RSSI: $rssi")
                }
                
                // Check if this is our host device
                if (deviceName == HostBleManager.HOST_DEVICE_NAME) {
                    Log.i(TAG, "Host device found: ${device.address} with RSSI: $rssi")
                    stopScan()
                    onDeviceFound(device)
                }
            }
            
            override fun onBatchScanResults(results: List<NordicScanResult>) {
                for (result in results) {
                    onScanResult(NordicScanSettings.CALLBACK_TYPE_ALL_MATCHES, result)
                }
            }
            
            override fun onScanFailed(errorCode: Int) {
                Log.e(TAG, "Scan failed with error: $errorCode")
                isScanning = false
                onScanFailed("Scan failed with error code: $errorCode")
            }
        }
        
        // Start scanning
        try {
            val callback = scanCallback
            if (callback != null) {
                scanner.startScan(scanFilters, scanSettings, callback)
                isScanning = true
                
                // Set timeout for scan
                android.os.Handler(android.os.Looper.getMainLooper()).postDelayed({
                    if (isScanning) {
                        Log.w(TAG, "Scan timeout - host device not found")
                        stopScan()
                        onScanFailed("Host device not found after ${SCAN_TIMEOUT_MS/1000} seconds")
                    }
                }, SCAN_TIMEOUT_MS)
            }
            
        } catch (e: Exception) {
            Log.e(TAG, "Failed to start scan", e)
            isScanning = false
            onScanFailed("Failed to start scan: ${e.message}")
        }
    }
    
    /**
     * Stop scanning
     */
    @SuppressLint("MissingPermission")
    fun stopScan() {
        val callback = scanCallback
        if (isScanning && callback != null) {
            Log.i(TAG, "Stopping scan")
            try {
                scanner.stopScan(callback)
            } catch (e: Exception) {
                Log.e(TAG, "Error stopping scan", e)
            }
            isScanning = false
            scanCallback = null
        }
    }
    
    /**
     * Check if currently scanning
     */
    fun isScanning(): Boolean = isScanning
}
