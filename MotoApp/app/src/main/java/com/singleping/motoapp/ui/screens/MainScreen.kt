package com.singleping.motoapp.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Bluetooth
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.singleping.motoapp.data.*
import com.singleping.motoapp.ui.components.RssiGraph
import com.singleping.motoapp.viewmodel.MotoAppBleViewModel
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive

// Helper functions
fun formatConnectionTime(timeInMillis: Long): String {
    val seconds = (timeInMillis / 1000) % 60
    val minutes = (timeInMillis / (1000 * 60)) % 60
    val hours = (timeInMillis / (1000 * 60 * 60))
    return String.format("%02d:%02d:%02d", hours, minutes, seconds)
}

fun getDistanceColor(rssi: Float): Color {
    return when {
        rssi > -50 -> Color(0xFF4CAF50) // Green - very close
        rssi > -70 -> Color(0xFFFF9800) // Orange - medium
        else -> Color(0xFFF44336) // Red - far
    }
}

@Composable
fun MainScreen(
    viewModel: MotoAppBleViewModel,
    onViewLogs: () -> Unit = {}
) {
    val connectionState by viewModel.connectionState.collectAsState()
    val streamState by viewModel.streamState.collectAsState()
    val rssiHistory by viewModel.rssiHistory.collectAsState()
    val distanceData by viewModel.distanceData.collectAsState()
    val hostInfo by viewModel.hostInfo.collectAsState()
    val mipeStatus by viewModel.mipeStatus.collectAsState()
    val logStats by viewModel.logStats.collectAsState()
    
    // Update connection time display
    var connectionTimeDisplay by remember { mutableStateOf("00:00:00") }
    LaunchedEffect(connectionState.connectionTime) {
        while (isActive && connectionState.isConnected) {
            connectionTimeDisplay = formatConnectionTime(connectionState.connectionTime)
            delay(1000)
        }
    }
    
    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(MaterialTheme.colorScheme.background)
            .verticalScroll(rememberScrollState())
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        // Title
        Text(
            text = "MotoApp TMT1",
            fontSize = 24.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.fillMaxWidth(),
            textAlign = TextAlign.Center
        )
        
        // 1. Host Connection Section
        ConnectionSection(
            connectionState = connectionState,
            onToggleConnection = { viewModel.toggleConnection() }
        )
        
        // 2. Data Stream Control Section
        DataStreamSection(
            streamState = streamState,
            isConnected = connectionState.isConnected,
            onToggleStream = { viewModel.toggleDataStream() }
        )
        
        // 3. Real-Time RSSI Graph
        Card(
            modifier = Modifier.fillMaxWidth(),
            elevation = CardDefaults.cardElevation(defaultElevation = 4.dp)
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "RSSI Histogram",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Medium,
                    modifier = Modifier.padding(bottom = 8.dp)
                )
                RssiGraph(
                    rssiHistory = rssiHistory,
                    modifier = Modifier.fillMaxWidth()
                )
            }
        }

        // 4. Logging Banner
        LoggingBanner(
            logStats = logStats,
            onViewLogs = onViewLogs,
            onExportData = { /* TODO: Implement export functionality */ }
        )

        // 5. Host Status Section
        mipeStatus?.let {
            HostStatusSection(status = it)
        }
        
        // 6. Status Display Panel
        StatusPanel(
            connectionState = connectionState,
            streamState = streamState,
            hostInfo = hostInfo,
            connectionTimeDisplay = connectionTimeDisplay
        )
        
        // 7. Distance Calculation Section
        DistanceSection(
            distanceData = distanceData,
            currentRssi = hostInfo.signalStrength
        )
    }
}

@Composable
fun ConnectionSection(
    connectionState: ConnectionState,
    onToggleConnection: () -> Unit
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(defaultElevation = 4.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            // Connection status indicator
            Text(
                text = when {
                    connectionState.isConnecting -> "Scanning for Host..."
                    connectionState.isConnected -> "Connected to Host"
                    else -> "Not Connected"
                },
                fontSize = 16.sp,
                fontWeight = FontWeight.Medium,
                color = when {
                    connectionState.isConnected -> Color(0xFF4CAF50)
                    connectionState.isConnecting -> Color(0xFFFF9800)
                    else -> MaterialTheme.colorScheme.onSurfaceVariant
                },
                modifier = Modifier.padding(bottom = 12.dp)
            )
            
            Button(
                onClick = onToggleConnection,
                modifier = Modifier
                    .fillMaxWidth()
                    .height(56.dp),
                colors = ButtonDefaults.buttonColors(
                    containerColor = when {
                        connectionState.isConnected -> Color.Red
                        connectionState.isConnecting -> Color.Gray
                        else -> Color(0xFF2196F3)
                    }
                ),
                enabled = !connectionState.isConnecting
            ) {
                Icon(
                    imageVector = Icons.Default.Bluetooth,
                    contentDescription = "Bluetooth",
                    modifier = Modifier.padding(end = 8.dp)
                )
                Text(
                    text = when {
                        connectionState.isConnecting -> "SCANNING..."
                        connectionState.isConnected -> "DISCONNECT HOST"
                        else -> "CONNECT TO HOST"
                    },
                    fontSize = 16.sp,
                    fontWeight = FontWeight.Bold
                )
            }
            
            // Show device info when connected
            if (connectionState.isConnected && connectionState.deviceName.isNotEmpty()) {
                Text(
                    text = "Device: ${connectionState.deviceName}",
                    fontSize = 12.sp,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                    modifier = Modifier.padding(top = 8.dp)
                )
            }
        }
    }
}

@Composable
fun LoggingBanner(
    logStats: LogStats,
    onViewLogs: () -> Unit,
    onExportData: () -> Unit
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(defaultElevation = 4.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            Text(
                text = "Data Logging",
                fontSize = 18.sp,
                fontWeight = FontWeight.Medium
            )
            
            // Status and Statistics Row
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                // Sample Counter
                Column {
                    Text(
                        text = "📊 ${logStats.totalSamples} Samples",
                        fontSize = 14.sp,
                        fontWeight = FontWeight.Medium
                    )
                    if (logStats.samplesPerSecond > 0) {
                        Text(
                            text = "${String.format("%.1f", logStats.samplesPerSecond)}/sec",
                            fontSize = 12.sp,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }
                }
                
                // Status Indicator
                Text(
                    text = if (logStats.isLogging) "✅ Active" else "⏸️ Paused",
                    fontSize = 12.sp,
                    color = if (logStats.isLogging) Color(0xFF4CAF50) else Color.Gray
                )
            }
            
            // Action Buttons
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                // View Logs Button
                Button(
                    onClick = onViewLogs,
                    modifier = Modifier.weight(1f),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = Color(0xFF2196F3)
                    ),
                    enabled = logStats.totalSamples > 0
                ) {
                    Text("👁️ View Logs")
                }
                
                // Export Button
                Button(
                    onClick = onExportData,
                    modifier = Modifier.weight(1f),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = Color(0xFF4CAF50)
                    ),
                    enabled = logStats.totalSamples > 0
                ) {
                    Text("💾 Export")
                }
            }
        }
    }
}

@Composable
fun DataStreamSection(
    streamState: StreamState,
    isConnected: Boolean,
    onToggleStream: () -> Unit
) {
    Button(
        onClick = onToggleStream,
        modifier = Modifier
            .fillMaxWidth()
            .height(48.dp),
        colors = ButtonDefaults.buttonColors(
            containerColor = if (streamState.isStreaming) Color.Red else Color(0xFF4CAF50),
            disabledContainerColor = Color.Gray
        ),
        enabled = isConnected
    ) {
        Text(
            text = if (streamState.isStreaming) "STOP DATA STREAM" else "START DATA STREAM",
            fontSize = 14.sp,
            fontWeight = FontWeight.Medium
        )
    }
}

@Composable
fun StatusPanel(
    connectionState: ConnectionState,
    streamState: StreamState,
    hostInfo: HostInfo,
    connectionTimeDisplay: String
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(defaultElevation = 4.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Text(
                text = "Status Display",
                fontSize = 18.sp,
                fontWeight = FontWeight.Medium
            )
            
            Divider()
            
            // BLE Connection Status
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Host Status:", fontWeight = FontWeight.Medium)
                Text(
                    text = when {
                        connectionState.isConnecting -> "Searching"
                        connectionState.isConnected -> "Connected"
                        else -> "Disconnected"
                    },
                    color = when {
                        connectionState.isConnected -> Color(0xFF4CAF50)
                        connectionState.isConnecting -> Color(0xFFFF9800)
                        else -> Color.Red
                    }
                )
            }
            
            // Data Stream Status
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Data Stream:", fontWeight = FontWeight.Medium)
                Text(
                    text = if (streamState.isStreaming) "Active" else "Stopped",
                    color = if (streamState.isStreaming) Color(0xFF4CAF50) else Color.Gray
                )
            }
            
            // Update Rate
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Update Rate:", fontWeight = FontWeight.Medium)
                Text("${streamState.updateRate}ms")
            }
            
            // Packets Received
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Packets Received:", fontWeight = FontWeight.Medium)
                Text("${streamState.packetsReceived}")
            }
            
            Divider()
            
            // Host Device Information
            Text(
                text = "Host Device Information",
                fontSize = 16.sp,
                fontWeight = FontWeight.Medium
            )
            
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Device Name:")
                Text(hostInfo.deviceName, fontWeight = FontWeight.Medium)
            }
            
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Battery Level:")
                Text(hostInfo.batteryLevel)
            }
            
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Signal Strength:")
                Text("${String.format("%.1f", hostInfo.signalStrength)} dBm")
            }
            
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Connected Time:")
                Text(connectionTimeDisplay)
            }
        }
    }
}

@Composable
fun DistanceSection(
    distanceData: DistanceData,
    currentRssi: Float
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(defaultElevation = 4.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            Text(
                text = "Distance Calculation",
                fontSize = 18.sp,
                fontWeight = FontWeight.Medium
            )
            
            // Current Distance Display
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(
                        color = Color.Black.copy(alpha = 0.05f),
                        shape = MaterialTheme.shapes.medium
                    )
                    .padding(16.dp),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    text = if (distanceData.currentDistance > 0) {
                        "${String.format("%.2f", distanceData.currentDistance)} m"
                    } else {
                        "-- m"
                    },
                    fontSize = 48.sp,
                    fontWeight = FontWeight.Bold,
                    color = getDistanceColor(currentRssi)
                )
            }
            
            // Distance Calculation Info
            Column(
                verticalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text("Algorithm Used:")
                    Text("RSSI-Based (Simulated)", fontWeight = FontWeight.Medium)
                }
                
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text("Confidence:")
                    Text("±${distanceData.confidence}m")
                }
                
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text("Last Updated:")
                    Text(
                        if (distanceData.lastUpdated > 0) {
                            val elapsed = (System.currentTimeMillis() - distanceData.lastUpdated) / 1000f
                            "${String.format("%.1f", elapsed)}s ago"
                        } else {
                            "Never"
                        }
                    )
                }
            }
            
            Divider()
            
            // Distance Statistics
            Text(
                text = "Distance Statistics",
                fontSize = 16.sp,
                fontWeight = FontWeight.Medium
            )
            
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Average Distance:")
                Text(
                    if (distanceData.averageDistance > 0) {
                        "${String.format("%.1f", distanceData.averageDistance)}m"
                    } else "--"
                )
            }
            
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Min Distance:")
                Text(
                    if (distanceData.minDistance < Float.MAX_VALUE) {
                        "${String.format("%.1f", distanceData.minDistance)}m"
                    } else "--"
                )
            }
            
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Max Distance:")
                Text(
                    if (distanceData.maxDistance > 0) {
                        "${String.format("%.1f", distanceData.maxDistance)}m"
                    } else "--"
                )
            }
            
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Sample Count:")
                Text("${distanceData.sampleCount}")
            }
        }
    }
}

@Composable
fun HostStatusSection(status: MipeStatus) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(defaultElevation = 4.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Text(
                text = "Host Status",
                fontSize = 18.sp,
                fontWeight = FontWeight.Medium
            )
            Divider()
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Mipe Connection:")
                Text(
                    text = status.connectionState,
                    fontWeight = FontWeight.Bold,
                    color = when (status.connectionState) {
                        "Connected" -> Color(0xFF4CAF50)
                        "Scanning" -> Color(0xFFFF9800)
                        else -> Color.Red
                    }
                )
            }
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Mipe RSSI:")
                Text("${status.rssi} dBm")
            }
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Mipe Address:")
                Text(status.deviceAddress ?: "N/A")
            }
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Connection Duration:")
                Text("${status.connectionDuration}s")
            }
        }
    }
}
