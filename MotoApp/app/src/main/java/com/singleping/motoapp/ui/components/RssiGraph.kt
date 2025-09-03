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
import com.singleping.motoapp.data.RssiData

@Composable
fun RssiGraph(
    rssiHistory: List<RssiData>,
    modifier: Modifier = Modifier
) {
    Box(
        modifier = modifier
            .fillMaxWidth()
            .height(250.dp)
            .background(Color(0xFFF5F5F5))
            .padding(8.dp)
    ) {
        // Y-axis labels (calibrated with 5 dB offset)
        Column(
            modifier = Modifier
                .align(Alignment.CenterStart)
                .width(40.dp)
                .fillMaxHeight(),
            verticalArrangement = Arrangement.SpaceBetween
        ) {
            Text("-25", fontSize = 10.sp, color = Color.Gray)
            Text("-35", fontSize = 10.sp, color = Color.Gray)
            Text("-45", fontSize = 10.sp, color = Color.Gray)
            Text("-55", fontSize = 10.sp, color = Color.Gray)
            Text("-65", fontSize = 10.sp, color = Color.Gray)
            Text("-75", fontSize = 10.sp, color = Color.Gray)
            Text("-85", fontSize = 10.sp, color = Color.Gray)
            Text("-95", fontSize = 10.sp, color = Color.Gray)
            Text("-105", fontSize = 10.sp, color = Color.Gray)
        }
        
        // Graph canvas
        Canvas(
            modifier = Modifier
                .fillMaxSize()
                .padding(start = 45.dp, bottom = 20.dp)
        ) {
            drawRssiGraph(rssiHistory)
        }
        
        // X-axis label
        Text(
            text = "Time (last 30 seconds)",
            fontSize = 10.sp,
            color = Color.Gray,
            modifier = Modifier
                .align(Alignment.BottomCenter)
                .padding(bottom = 4.dp),
            textAlign = TextAlign.Center
        )
    }
}

private fun DrawScope.drawRssiGraph(rssiHistory: List<RssiData>) {
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
    
    // Draw RSSI line (calibrated with 5 dB offset)
    if (rssiHistory.size > 1) {
        val path = Path()
        val minRssi = -105f  // Adjusted from -100f
        val maxRssi = -25f   // Adjusted from -20f
        val rssiRange = maxRssi - minRssi
        
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
        
        // Draw the line
        drawPath(
            path = path,
            color = Color(0xFFFF6B35), // Orange/Red color
            style = Stroke(width = 2.dp.toPx())
        )
        
        // Draw dots for recent points
        val recentPoints = rssiHistory.takeLast(10)
        recentPoints.forEachIndexed { index, rssiData ->
            val globalIndex = rssiHistory.size - recentPoints.size + index
            val x = (globalIndex.toFloat() / 299f) * width
            val normalizedRssi = (rssiData.value - minRssi) / rssiRange
            val y = height * (1f - normalizedRssi)
            
            drawCircle(
                color = Color(0xFFFF6B35),
                radius = 3.dp.toPx(),
                center = Offset(x, y)
            )
        }
    }
}
