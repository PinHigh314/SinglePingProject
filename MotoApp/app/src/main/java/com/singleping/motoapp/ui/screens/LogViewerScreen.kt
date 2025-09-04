package com.singleping.motoapp.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.singleping.motoapp.viewmodel.MotoAppBleViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun LogViewerScreen(
    viewModel: MotoAppBleViewModel,
    onBack: () -> Unit
) {
    val loggingData by viewModel.loggingData.collectAsState()
    val logHistory by viewModel.logHistory.collectAsState()

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Log Viewer") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                },
                actions = {
                    // Clear Logs Button
                    if (loggingData.isNotEmpty() || logHistory.isNotEmpty()) {
                        IconButton(
                            onClick = { viewModel.clearLogData() },
                            enabled = loggingData.isNotEmpty() || logHistory.isNotEmpty()
                        ) {
                            Icon(
                                imageVector = Icons.Default.Delete,
                                contentDescription = "Clear Logs"
                            )
                        }
                    }
                }
            )
        }
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .padding(innerPadding)
                .fillMaxSize()
        ) {
            // Log Statistics
            Card(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(16.dp)
            ) {
                Column(
                    modifier = Modifier.padding(16.dp)
                ) {
                    Text(
                        text = "Log Statistics",
                        fontSize = 18.sp,
                        fontWeight = FontWeight.Medium
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    Text(text = "Total Samples: ${loggingData.size}")
                    Text(text = "System Logs: ${logHistory.size} entries")
                }
            }

            // Calibration Reference Values
            Card(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 16.dp, vertical = 8.dp)
            ) {
                Column(
                    modifier = Modifier.padding(16.dp)
                ) {
                    Text(
                        text = "Calibration Reference",
                        fontSize = 18.sp,
                        fontWeight = FontWeight.Medium
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    
                    // Display calibration values in two columns
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        // First column (1-4m)
                        Column(modifier = Modifier.weight(1f)) {
                            Text(text = "1m: -45.0 dBm", fontSize = 14.sp)
                            Text(text = "2m: -52.0 dBm", fontSize = 14.sp)
                            Text(text = "3m: -57.0 dBm", fontSize = 14.sp)
                            Text(text = "4m: -60.0 dBm", fontSize = 14.sp)
                        }
                        
                        // Second column (5-8m)
                        Column(modifier = Modifier.weight(1f)) {
                            Text(text = "5m: -63.0 dBm", fontSize = 14.sp)
                            Text(text = "6m: -65.0 dBm", fontSize = 14.sp)
                            Text(text = "7m: -67.0 dBm", fontSize = 14.sp)
                            Text(text = "8m: -69.0 dBm", fontSize = 14.sp)
                        }
                    }
                }
            }

            // Log Data List
            Text(
                text = "Logged Data Samples",
                fontSize = 16.sp,
                fontWeight = FontWeight.Medium,
                modifier = Modifier.padding(16.dp)
            )

            if (loggingData.isEmpty()) {
                Text(
                    text = "No data logged yet. Start data streaming to collect samples.",
                    modifier = Modifier.padding(16.dp)
                )
            } else {
                LazyColumn(
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(1f)
                        .padding(horizontal = 16.dp)
                ) {
                    items(loggingData.reversed()) { logData ->
                        LogDataItem(logData = logData)
                        HorizontalDivider()
                    }
                }
            }

            // System Logs
            Text(
                text = "System Logs",
                fontSize = 16.sp,
                fontWeight = FontWeight.Medium,
                modifier = Modifier.padding(16.dp)
            )

            if (logHistory.isEmpty()) {
                Text(
                    text = "No system logs available.",
                    modifier = Modifier.padding(16.dp)
                )
            } else {
                LazyColumn(
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(1f)
                        .padding(horizontal = 16.dp)
                ) {
                    items(logHistory.reversed()) { logEntry ->
                        SystemLogItem(logEntry = logEntry)
                        HorizontalDivider()
                    }
                }
            }
        }
    }
}

@Composable
fun LogDataItem(logData: com.singleping.motoapp.data.LogData) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(8.dp)
    ) {
        Text(
            text = "RSSI: ${String.format("%.1f", logData.rssi)} dBm",
            fontWeight = FontWeight.Medium
        )
        Text(text = "Distance: ${String.format("%.2f", logData.distance)} m")
        Text(
            text = "Time: ${java.text.SimpleDateFormat("HH:mm:ss.SSS").format(java.util.Date(logData.timestamp))}",
            fontSize = 12.sp
        )
    }
}

@Composable
fun SystemLogItem(logEntry: String) {
    Text(
        text = logEntry,
        modifier = Modifier
            .fillMaxWidth()
            .padding(8.dp),
        fontSize = 14.sp
    )
}
