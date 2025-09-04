package com.singleping.motoapp.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.text.font.FontWeight
import com.singleping.motoapp.data.RssiData
import com.singleping.motoapp.data.calculateDistance

@Composable
fun RssiGraph(
    rssiHistory: List<RssiData>,
    filteredRssiHistory: List<RssiData> = emptyList(),
    modifier: Modifier = Modifier
) {
    Box(
        modifier = modifier
            .fillMaxWidth()
            .height(250.dp)
            .background(Color(0xFFF5F5F5))
            .padding(8.dp)
    ) {
        // Left Y-axis labels (RSSI in dBm)
        Column(
            modifier = Modifier
                .align(Alignment.CenterStart)
                .width(40.dp)
                .fillMaxHeight(),
            verticalArrangement = Arrangement.SpaceBetween
        ) {
            Text("-20", fontSize = 10.sp, color = Color.Blue)
            Text("-30", fontSize = 10.sp, color = Color.Blue)
            Text("-40", fontSize = 10.sp, color = Color.Blue)
            Text("-50", fontSize = 10.sp, color = Color.Blue)
            Text("-60", fontSize = 10.sp, color = Color.Blue)
            Text("-70", fontSize = 10.sp, color = Color.Blue)
            Text("-80", fontSize = 10.sp, color = Color.Blue)
            Text("-90", fontSize = 10.sp, color = Color.Blue)
            Text("-100", fontSize = 10.sp, color = Color.Blue)
        }
        
        // Right Y-axis labels (placeholder for dual axis)
        Column(
            modifier = Modifier
                .align(Alignment.CenterEnd)
                .width(40.dp)
                .fillMaxHeight(),
            verticalArrangement = Arrangement.SpaceBetween
        ) {
            Text("0", fontSize = 10.sp, color = Color.Gray)
            Text("20", fontSize = 10.sp, color = Color.Gray)
            Text("40", fontSize = 10.sp, color = Color.Gray)
            Text("60", fontSize = 10.sp, color = Color.Gray)
            Text("80", fontSize = 10.sp, color = Color.Gray)
            Text("100", fontSize = 10.sp, color = Color.Gray)
            Text("120", fontSize = 10.sp, color = Color.Gray)
            Text("140", fontSize = 10.sp, color = Color.Gray)
            Text("160", fontSize = 10.sp, color = Color.Gray)
        }
        
        // Graph canvas
        Canvas(
            modifier = Modifier
                .fillMaxSize()
                .padding(start = 45.dp, end = 45.dp, bottom = 8.dp)
        ) {
            drawRssiGraph(rssiHistory, filteredRssiHistory)
        }
    }
}

private fun DrawScope.drawRssiGraph(
    rssiHistory: List<RssiData>,
    filteredRssiHistory: List<RssiData>
) {
    val width = size.width
    val height = size.height
    
    // Draw grid lines
    val gridColor = Color.LightGray.copy(alpha = 0.5f)
    
    // Horizontal grid lines (RSSI values)
    for (i in 0..8) {
        val y = height * i / 8
        drawLine(
            color = gridColor,
            start = Offset(0f, y),
            end = Offset(width, y),
            strokeWidth = 0.5.dp.toPx()
        )
    }
    
    // Vertical grid lines (time)
    for (i in 0..6) {
        val x = width * i / 6
        drawLine(
            color = gridColor,
            start = Offset(x, 0f),
            end = Offset(x, height),
            strokeWidth = 0.5.dp.toPx()
        )
    }
    
    // Common scaling parameters
    val minRssi = -100f  // Bottom of scale
    val maxRssi = -20f   // Top of scale (matches axis labels)
    val rssiRange = maxRssi - minRssi  // 80 dB range
    
    // Draw raw RSSI line (blue)
    if (rssiHistory.size > 1) {
        val path = Path()
        
        rssiHistory.forEachIndexed { index, rssiData ->
            val x = (index.toFloat() / (299f)) * width // Max 300 points
            val normalizedRssi = (rssiData.value - minRssi) / rssiRange
            val y = height * (1f - normalizedRssi)
            
            if (index == 0) {
                path.moveTo(x, y)
            } else {
                path.lineTo(x, y)
            }
        }
        
        // Draw raw RSSI line in blue with some transparency
        drawPath(
            path = path,
            color = Color.Blue.copy(alpha = 0.5f),
            style = Stroke(width = 1.5.dp.toPx())
        )
    }
    
    // Draw Kalman filtered RSSI line (red/green)
    if (filteredRssiHistory.size > 1) {
        val filteredPath = Path()
        
        filteredRssiHistory.forEachIndexed { index, rssiData ->
            val x = (index.toFloat() / (299f)) * width // Max 300 points
            val normalizedRssi = (rssiData.value - minRssi) / rssiRange
            val y = height * (1f - normalizedRssi)
            
            if (index == 0) {
                filteredPath.moveTo(x, y)
            } else {
                filteredPath.lineTo(x, y)
            }
        }
        
        // Draw filtered line in red (or green) with full opacity
        drawPath(
            path = filteredPath,
            color = Color.Red,
            style = Stroke(width = 2.dp.toPx())
        )
        
        // Draw dots for recent filtered points
        val recentFilteredPoints = filteredRssiHistory.takeLast(5)
        recentFilteredPoints.forEachIndexed { index, rssiData ->
            val globalIndex = filteredRssiHistory.size - recentFilteredPoints.size + index
            val x = (globalIndex.toFloat() / 299f) * width
            val normalizedRssi = (rssiData.value - minRssi) / rssiRange
            val y = height * (1f - normalizedRssi)
            
            drawCircle(
                color = Color.Red,
                radius = 3.dp.toPx(),
                center = Offset(x, y)
            )
        }
    }
}
