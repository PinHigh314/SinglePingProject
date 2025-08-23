package com.singleping.motoapp.ui.screens

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.util.Log
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.ContextCompat
import kotlinx.coroutines.delay
import no.nordicsemi.android.support.v18.scanner.BluetoothLeScannerCompat
import no.nordicsemi.android.support.v18.scanner.ScanCallback
import no.nordicsemi.android.support.v18.scanner.ScanResult
import no.nordicsemi.android.support.v18.scanner.ScanSettings

data class BleDevice(
    val name: String?,
    val address: String,
    val rssi: Int,
    val serviceUuids: List<String> = emptyList()
)

@SuppressLint("MissingPermission")
@Composable
fun DebugScreen() {
    val context = LocalContext.current
    var isScanning by remember { mutableStateOf(false) }
    var devices by remember { mutableStateOf<List<BleDevice>>(emptyList()) }
    var scanStatus by remember { mutableStateOf("Ready to scan") }
    val deviceMap = remember { mutableMapOf<String, BleDevice>() }
    var scanCount by remember { mutableStateOf(0) }
    
    // Check Bluetooth status
    val bluetoothManager = remember { context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager }
    val bluetoothAdapter = remember { bluetoothManager.adapter }
    val isBluetoothEnabled = bluetoothAdapter?.isEnabled ?: false
    
    // Check permissions
    val hasBluetoothScanPermission = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
        ContextCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED
    } else true
    
    val hasBluetoothConnectPermission = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
        ContextCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED
    } else true
    
    val hasLocationPermission = 
        ContextCompat.checkSelfPermission(context, Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED ||
        ContextCompat.checkSelfPermission(context, Manifest.permission.ACCESS_COARSE_LOCATION) == PackageManager.PERMISSION_GRANTED
    
    val allPermissionsGranted = hasBluetoothScanPermission && hasBluetoothConnectPermission && hasLocationPermission
    
    val scanner = remember { BluetoothLeScannerCompat.getScanner() }
    var scanCallback by remember { mutableStateOf<ScanCallback?>(null) }
    
    fun parseServiceUuids(scanRecord: ByteArray?): List<String> {
        val uuids = mutableListOf<String>()
        if (scanRecord == null) return uuids
        
        var currentPos = 0
        while (currentPos < scanRecord.size) {
            val length = scanRecord[currentPos].toInt() and 0xFF
            if (length == 0) break
            
            val dataLength = length - 1
            if (currentPos + length >= scanRecord.size) break
            
            val type = scanRecord[currentPos + 1].toInt() and 0xFF
            val data = scanRecord.sliceArray((currentPos + 2) until (currentPos + 1 + length))
            
            // Check for 128-bit UUID types
            if (type == 0x06 || type == 0x07) { // Complete or incomplete 128-bit UUIDs
                var i = 0
                while (i + 15 < data.size) {
                    val uuid = buildString {
                        for (j in 15 downTo 0) {
                            append(String.format("%02x", data[i + j]))
                            if (j == 12 || j == 10 || j == 8 || j == 6) append("-")
                        }
                    }
                    uuids.add(uuid)
                    i += 16
                }
            }
            
            currentPos += length + 1
        }
        
        return uuids
    }
    
    fun startScan() {
        if (isScanning) return
        
        if (!allPermissionsGranted) {
            scanStatus = "ERROR: Missing permissions! Check permission status below."
            Log.e("DebugScreen", "Missing permissions - Scan: $hasBluetoothScanPermission, Connect: $hasBluetoothConnectPermission, Location: $hasLocationPermission")
            return
        }
        
        if (!isBluetoothEnabled) {
            scanStatus = "ERROR: Bluetooth is disabled!"
            Log.e("DebugScreen", "Bluetooth is disabled")
            return
        }
        
        deviceMap.clear()
        devices = emptyList()
        scanCount = 0
        scanStatus = "Scanning for ALL BLE devices..."
        isScanning = true
        Log.i("DebugScreen", "Starting BLE scan with all permissions granted...")
        
        scanCallback = object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: ScanResult) {
                scanCount++
                val device = result.device
                val deviceName = device.name
                val deviceAddress = device.address
                val scanRecord = result.scanRecord?.bytes
                val serviceUuids = parseServiceUuids(scanRecord)
                
                Log.d("DebugScreen", "Scan result #$scanCount: name=$deviceName, address=$deviceAddress, rssi=${result.rssi}, uuids=$serviceUuids")
                
                val bleDevice = BleDevice(
                    name = deviceName,
                    address = deviceAddress,
                    rssi = result.rssi,
                    serviceUuids = serviceUuids
                )
                
                deviceMap[deviceAddress] = bleDevice
                devices = deviceMap.values.sortedByDescending { it.rssi }
                
                // Check for our target - exact match
                if (deviceName == "MIPE_HOST_A1B2") {
                    scanStatus = "✅ FOUND TARGET: $deviceName"
                    Log.i("DebugScreen", "TARGET DEVICE FOUND: $deviceName at $deviceAddress with UUIDs: $serviceUuids")
                } else if (deviceName != null) {
                    // Check for partial matches
                    if (deviceName.contains("MIPE", ignoreCase = true) || 
                        deviceName.contains("HOST", ignoreCase = true) ||
                        deviceName.contains("nRF", ignoreCase = true)) {
                        scanStatus = "Found potential device: $deviceName"
                        Log.i("DebugScreen", "Potential match: $deviceName")
                    }
                }
                
                // Also check for our service UUID
                if (serviceUuids.any { it.contains("12345678-1234-5678-1234-56789abcdef0", ignoreCase = true) }) {
                    Log.i("DebugScreen", "Found device with our service UUID: $deviceName ($deviceAddress)")
                    scanStatus = "Found device with SinglePing service UUID!"
                }
            }
            
            override fun onScanFailed(errorCode: Int) {
                val errorMsg = when (errorCode) {
                    ScanCallback.SCAN_FAILED_ALREADY_STARTED -> "Scan already started"
                    ScanCallback.SCAN_FAILED_APPLICATION_REGISTRATION_FAILED -> "App registration failed"
                    ScanCallback.SCAN_FAILED_INTERNAL_ERROR -> "Internal error"
                    ScanCallback.SCAN_FAILED_FEATURE_UNSUPPORTED -> "Feature unsupported"
                    else -> "Unknown error: $errorCode"
                }
                scanStatus = "Scan failed: $errorMsg"
                Log.e("DebugScreen", "Scan failed: $errorMsg")
                isScanning = false
            }
        }
        
        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .setReportDelay(0)
            .setUseHardwareBatchingIfSupported(false)
            .build()
        
        try {
            scanner.startScan(emptyList(), settings, scanCallback!!)
            Log.i("DebugScreen", "Scan started successfully")
        } catch (e: Exception) {
            scanStatus = "Failed to start scan: ${e.message}"
            Log.e("DebugScreen", "Failed to start scan", e)
            isScanning = false
        }
    }
    
    fun stopScan() {
        if (!isScanning) return
        
        scanCallback?.let {
            try {
                scanner.stopScan(it)
            } catch (e: Exception) {
                Log.e("DebugScreen", "Error stopping scan", e)
            }
        }
        isScanning = false
        scanStatus = "Scan stopped. Found ${devices.size} devices"
    }
    
    DisposableEffect(Unit) {
        onDispose {
            if (isScanning) {
                stopScan()
            }
        }
    }
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(
            text = "BLE Debug Scanner v3.4",
            fontSize = 24.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        // Permission Status Card
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 8.dp),
            colors = CardDefaults.cardColors(
                containerColor = if (allPermissionsGranted) 
                    Color(0xFF4CAF50).copy(alpha = 0.1f)
                else 
                    Color(0xFFF44336).copy(alpha = 0.1f)
            )
        ) {
            Column(
                modifier = Modifier.padding(12.dp)
            ) {
                Text(
                    text = "Permission Status",
                    fontWeight = FontWeight.Bold,
                    fontSize = 14.sp
                )
                Spacer(modifier = Modifier.height(4.dp))
                Text(
                    text = "Bluetooth Scan: ${if (hasBluetoothScanPermission) "✅" else "❌"}",
                    fontSize = 12.sp
                )
                Text(
                    text = "Bluetooth Connect: ${if (hasBluetoothConnectPermission) "✅" else "❌"}",
                    fontSize = 12.sp
                )
                Text(
                    text = "Location: ${if (hasLocationPermission) "✅" else "❌"}",
                    fontSize = 12.sp
                )
                Text(
                    text = "Bluetooth Enabled: ${if (isBluetoothEnabled) "✅" else "❌"}",
                    fontSize = 12.sp
                )
            }
        }
        
        // Scan Status Card
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 16.dp),
            colors = CardDefaults.cardColors(
                containerColor = if (isScanning) 
                    MaterialTheme.colorScheme.primaryContainer 
                else 
                    MaterialTheme.colorScheme.surfaceVariant
            )
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Scan Status",
                    fontWeight = FontWeight.Bold,
                    fontSize = 16.sp
                )
                Text(
                    text = scanStatus,
                    fontSize = 14.sp,
                    modifier = Modifier.padding(top = 4.dp)
                )
                Text(
                    text = "Devices found: ${devices.size}",
                    fontSize = 14.sp,
                    modifier = Modifier.padding(top = 4.dp)
                )
                Text(
                    text = "Scan callbacks: $scanCount",
                    fontSize = 12.sp,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
        
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 16.dp),
            horizontalArrangement = Arrangement.SpaceEvenly
        ) {
            Button(
                onClick = { startScan() },
                enabled = !isScanning && allPermissionsGranted && isBluetoothEnabled,
                modifier = Modifier.weight(1f).padding(end = 8.dp)
            ) {
                Text("Start Scan")
            }
            
            Button(
                onClick = { stopScan() },
                enabled = isScanning,
                modifier = Modifier.weight(1f).padding(start = 8.dp)
            ) {
                Text("Stop Scan")
            }
        }
        
        Text(
            text = "Looking for: MIPE_HOST_A1B2",
            fontSize = 12.sp,
            color = MaterialTheme.colorScheme.primary,
            modifier = Modifier.padding(bottom = 8.dp)
        )
        
        Divider(modifier = Modifier.padding(vertical = 8.dp))
        
        LazyColumn(
            modifier = Modifier.fillMaxSize()
        ) {
            items(devices) { device ->
                Card(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(vertical = 4.dp),
                    colors = CardDefaults.cardColors(
                        containerColor = when {
                            device.name == "MIPE_HOST_A1B2" -> MaterialTheme.colorScheme.primaryContainer
                            device.serviceUuids.any { it.contains("12345678-1234-5678-1234-56789abcdef0", ignoreCase = true) } -> 
                                MaterialTheme.colorScheme.secondaryContainer
                            device.name?.contains("MIPE") == true -> MaterialTheme.colorScheme.tertiaryContainer
                            device.name?.contains("nRF") == true -> MaterialTheme.colorScheme.tertiaryContainer
                            else -> MaterialTheme.colorScheme.surface
                        }
                    )
                ) {
                    Column(
                        modifier = Modifier.padding(12.dp)
                    ) {
                        Text(
                            text = device.name ?: "Unnamed Device",
                            fontWeight = FontWeight.Bold,
                            fontSize = 14.sp
                        )
                        Text(
                            text = device.address,
                            fontSize = 12.sp,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                        Text(
                            text = "RSSI: ${device.rssi} dBm",
                            fontSize = 12.sp,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                        if (device.serviceUuids.isNotEmpty()) {
                            Text(
                                text = "Services: ${device.serviceUuids.size}",
                                fontSize = 10.sp,
                                color = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                            device.serviceUuids.forEach { uuid ->
                                if (uuid.contains("12345678", ignoreCase = true)) {
                                    Text(
                                        text = "✅ SinglePing Service",
                                        fontSize = 10.sp,
                                        color = MaterialTheme.colorScheme.primary,
                                        fontWeight = FontWeight.Bold
                                    )
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
