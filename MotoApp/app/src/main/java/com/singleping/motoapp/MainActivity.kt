package com.singleping.motoapp

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.core.content.ContextCompat
import com.singleping.motoapp.ui.screens.LogViewerScreen
import com.singleping.motoapp.ui.screens.MainScreen
import com.singleping.motoapp.ui.theme.SinglePingMotoAppTheme
import com.singleping.motoapp.viewmodel.MotoAppBleViewModel

enum class Screen {
    MAIN, LOG_VIEWER
}

class MainActivity : ComponentActivity() {
    
    // Use the enhanced BLE ViewModel
    private val viewModel: MotoAppBleViewModel by viewModels()
    
    // Permission launcher for BLE permissions
    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { permissions ->
        val allGranted = permissions.values.all { it }
        if (allGranted) {
            Toast.makeText(this, "All BLE permissions granted", Toast.LENGTH_SHORT).show()
            // Permissions granted, BLE will initialize automatically
        } else {
            val deniedPermissions = permissions.filter { !it.value }.keys
            Toast.makeText(
                this, 
                "Missing permissions: ${deniedPermissions.joinToString(", ")}\nBLE scanning will not work without these permissions!", 
                Toast.LENGTH_LONG
            ).show()
            // BLE scanning will not work without permissions
        }
    }
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Initialize the data exporter for Google Drive
        viewModel.initializeExporter(this)
        
        // Check and request BLE permissions
        checkAndRequestPermissions()
        
        setContent {
            SinglePingMotoAppTheme {
                // Collect error messages
                val errorMessage by viewModel.errorMessage.collectAsState()
                
                // Show toast for error messages
                LaunchedEffect(errorMessage) {
                    errorMessage?.let {
                        Toast.makeText(this@MainActivity, it, Toast.LENGTH_SHORT).show()
                        // Clear error after showing
                        viewModel.clearError()
                    }
                }
                
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    // Navigation state
                    val currentScreen = remember { mutableStateOf(Screen.MAIN) }
                    
                    when (currentScreen.value) {
                        Screen.MAIN -> {
                            MainScreen(
                                viewModel = viewModel,
                                onViewLogs = { currentScreen.value = Screen.LOG_VIEWER }
                            )
                        }
                        Screen.LOG_VIEWER -> {
                            LogViewerScreen(
                                viewModel = viewModel,
                                onBack = { currentScreen.value = Screen.MAIN }
                            )
                        }
                    }
                }
            }
        }
    }
    
    private fun checkAndRequestPermissions(): Boolean {
        val requiredPermissions = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            // Android 12+ requires new Bluetooth permissions AND location
            arrayOf(
                Manifest.permission.BLUETOOTH_SCAN,
                Manifest.permission.BLUETOOTH_CONNECT,
                Manifest.permission.ACCESS_FINE_LOCATION  // Still needed for BLE scanning
            )
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            // Android 6-11 requires location permission for BLE scanning
            arrayOf(
                Manifest.permission.ACCESS_FINE_LOCATION
            )
        } else {
            // Android 5 and below don't need runtime permissions
            emptyArray()
        }
        
        if (requiredPermissions.isNotEmpty()) {
            val permissionsToRequest = requiredPermissions.filter {
                ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
            }
            
            if (permissionsToRequest.isNotEmpty()) {
                // Show explanation for why we need these permissions
                Toast.makeText(
                    this,
                    "BLE scanning requires Bluetooth and Location permissions",
                    Toast.LENGTH_LONG
                ).show()
                permissionLauncher.launch(permissionsToRequest.toTypedArray())
                return false // Permissions not yet granted
            }
        }
        
        // All permissions already granted
        return true
    }
    
    override fun onDestroy() {
        super.onDestroy()
        // BLE resources will be cleaned up automatically
    }
}
