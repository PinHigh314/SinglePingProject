package com.singleping.motoapp.ui.screens

import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.singleping.motoapp.viewmodel.MotoAppBleViewModel

@Composable
fun CalibrationScreen(viewModel: MotoAppBleViewModel, onBack: () -> Unit) {
    val calibrationState by viewModel.calibrationState.collectAsState()
    val connectionState by viewModel.connectionState.collectAsState()
    val mipeStatus by viewModel.mipeStatus.collectAsState()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
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
    }
}
