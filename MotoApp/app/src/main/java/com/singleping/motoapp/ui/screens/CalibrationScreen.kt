package com.singleping.motoapp.ui.screens

import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.singleping.motoapp.viewmodel.MotoAppBleViewModel

@Composable
fun CalibrationScreen(viewModel: MotoAppBleViewModel, onBack: () -> Unit) {
    val calibrationState by viewModel.calibrationState.collectAsState()
    val connectionState by viewModel.connectionState.collectAsState()
    val mipeStatus by viewModel.mipeStatus.collectAsState()
    val scrollState = rememberScrollState()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
            .verticalScroll(scrollState),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Button(onClick = onBack) {
            Text(text = "Back to Main Screen")
        }
        Row {
            Text(
                text = "Host Connection: ${if (connectionState.isConnected) "Connected" else "Disconnected"}",
                color = if (connectionState.isConnected) Color.Green else Color.Red
            )
            Spacer(modifier = Modifier.width(16.dp))
            Text(
                text = "Mipe Connection: ${if (mipeStatus != null) "Connected" else "Disconnected"}",
                color = if (mipeStatus != null) Color.Green else Color.Red
            )
        }

        Spacer(modifier = Modifier.height(16.dp))

        val distances = listOf(1, 2, 5, 10, 20, 40, 70, 100)
        distances.chunked(4).forEach { rowDistances ->
            Row(
                horizontalArrangement = Arrangement.spacedBy(8.dp),
                modifier = Modifier.fillMaxWidth()
            ) {
                rowDistances.forEach { distance ->
                    Button(
                        onClick = { viewModel.selectCalibrationDistance(distance) },
                        modifier = Modifier.weight(1f),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = if (calibrationState.selectedDistance == distance) Color.Green else Color.Gray
                        )
                    ) {
                        Text(text = "$distance m")
                    }
                }
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        Button(
            onClick = { viewModel.startCalibration() },
            modifier = Modifier.fillMaxWidth(),
            enabled = calibrationState.selectedDistance != 0
        ) {
            Text(text = "Calibration Start")
        }

        Spacer(modifier = Modifier.height(16.dp))

        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(200.dp)
                .border(
                    width = 2.dp,
                    color = if (calibrationState.isCollecting) Color.Green else Color.Red
                )
                .padding(8.dp)
        ) {
            Column {
                calibrationState.data.takeLast(10).forEachIndexed { index, data ->
                    Text(
                        text = "${index + 1}: Timestamp: ${data.timestamp}, RSSI: ${data.rssi}, Mipe mV: ${data.mipeBatteryMv}",
                        fontSize = 12.sp,
                        lineHeight = 12.sp
                    )
                }
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        Button(
            onClick = { viewModel.cancelCalibration() },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(text = "Cancel")
        }

        Spacer(modifier = Modifier.height(16.dp))

        var comment by remember { mutableStateOf("") }
        TextField(
            value = comment,
            onValueChange = { comment = it },
            modifier = Modifier
                .fillMaxWidth()
                .height(150.dp),
            label = { Text("Comment") },
            enabled = calibrationState.isComplete
        )

        Spacer(modifier = Modifier.height(16.dp))

        Button(
            onClick = { viewModel.completeCalibration(comment) },
            modifier = Modifier.fillMaxWidth(),
            enabled = calibrationState.isComplete
        ) {
            Text(text = "Complete")
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Calibration Log Table
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(vertical = 8.dp)
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Calibration Log - Logarithmic Regression Values",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Bold
                )
                Text(
                    text = "Formula: RSSI = A × log₁₀(distance) + B",
                    fontSize = 14.sp,
                    color = Color.Gray,
                    modifier = Modifier.padding(vertical = 4.dp)
                )
                
                Spacer(modifier = Modifier.height(12.dp))
                
                // Table Header
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text(
                        text = "Distance\n(m)",
                        fontSize = 14.sp,
                        fontWeight = FontWeight.Bold,
                        textAlign = TextAlign.Center,
                        modifier = Modifier.weight(1f)
                    )
                    Text(
                        text = "Status",
                        fontSize = 14.sp,
                        fontWeight = FontWeight.Bold,
                        textAlign = TextAlign.Center,
                        modifier = Modifier.weight(1f)
                    )
                    Text(
                        text = "Avg Filtered\nRSSI (dBm)",
                        fontSize = 14.sp,
                        fontWeight = FontWeight.Bold,
                        textAlign = TextAlign.Center,
                        modifier = Modifier.weight(1.2f)
                    )
                }
                
                HorizontalDivider(
                    modifier = Modifier.padding(vertical = 8.dp),
                    thickness = 1.dp,
                    color = Color.Gray
                )
                
                // Table Data - Dynamic values from actual calibrations
                val calibrationDistances = listOf(1, 2, 5, 10, 20, 40, 70, 100)
                val completedCalibrations = calibrationState.completedCalibrations
                
                calibrationDistances.forEach { distance ->
                    val calibration = completedCalibrations[distance]
                    val isCalibrated = calibration != null
                    
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(vertical = 4.dp),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text(
                            text = "$distance",
                            fontSize = 14.sp,
                            textAlign = TextAlign.Center,
                            modifier = Modifier.weight(1f)
                        )
                        Text(
                            text = if (isCalibrated) "✓" else "--",
                            fontSize = 14.sp,
                            textAlign = TextAlign.Center,
                            color = if (isCalibrated) Color(0xFF4CAF50) else Color.Gray,
                            fontWeight = if (isCalibrated) FontWeight.Bold else FontWeight.Normal,
                            modifier = Modifier.weight(1f)
                        )
                        Text(
                            text = if (isCalibrated) {
                                String.format("%.2f", calibration!!.averageFilteredRssi)
                            } else {
                                "Not calibrated"
                            },
                            fontSize = 14.sp,
                            textAlign = TextAlign.Center,
                            fontWeight = if (isCalibrated) FontWeight.Medium else FontWeight.Normal,
                            color = if (isCalibrated) Color(0xFF2E7D32) else Color.Gray,
                            modifier = Modifier.weight(1.2f)
                        )
                    }
                }
                
                HorizontalDivider(
                    modifier = Modifier.padding(vertical = 8.dp),
                    thickness = 1.dp,
                    color = Color.Gray
                )
                
                // Show calibration progress
                val calibratedCount = completedCalibrations.size
                val totalCount = calibrationDistances.size
                Text(
                    text = "Calibrated: $calibratedCount / $totalCount distances",
                    fontSize = 14.sp,
                    color = if (calibratedCount == totalCount) Color(0xFF4CAF50) else Color.Gray,
                    textAlign = TextAlign.Center,
                    modifier = Modifier.fillMaxWidth().padding(top = 8.dp)
                )
                
                // Add persistence indicator
                if (calibratedCount > 0) {
                    Text(
                        text = "✓ Calibrations saved to device storage",
                        fontSize = 12.sp,
                        color = Color(0xFF4CAF50),
                        textAlign = TextAlign.Center,
                        modifier = Modifier.fillMaxWidth().padding(top = 4.dp)
                    )
                }
                
                Text(
                    text = "These values are used for logarithmic regression\nto estimate distances between calibration points.",
                    fontSize = 12.sp,
                    color = Color.Gray,
                    textAlign = TextAlign.Center,
                    modifier = Modifier.fillMaxWidth().padding(top = 8.dp)
                )
                
                // Clear All Calibrations Button
                if (calibratedCount > 0) {
                    Spacer(modifier = Modifier.height(12.dp))
                    Button(
                        onClick = { viewModel.clearAllCalibrations() },
                        modifier = Modifier.fillMaxWidth(),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = Color(0xFFD32F2F) // Red color for destructive action
                        )
                    ) {
                        Text(
                            text = "Clear All Calibrations",
                            color = Color.White
                        )
                    }
                }
            }
        }
    }
}
