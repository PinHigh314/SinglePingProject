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
import com.singleping.motoapp.data.DistancePoint
import kotlin.math.pow

@Composable
fun DualAxisRssiGraph(
    rssiHistory: List<RssiData>,
    distanceHistory: List<DistancePoint>,
    modifier: Modifier = Modifier
) {
    Box(
        modifier = modifier
            .fillMaxWidth()
            .height(300.dp)
            .background(Color(0xFFF5F5F5))
            .padding(8.dp)
    ) {
        // Left Y-axis labels (RSSI in dBm)
        Column(
            modifier = Modifier
                .align(Alignment.CenterStart)
                .width(45.dp)
                .fillMaxHeight()
                .padding(top = 10.dp, bottom = 25.dp),
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
        
        // Right Y-axis labels (Distance in meters)
        Column(
            modifier = Modifier
                .align(Alignment.CenterEnd)
                .width(50.dp)
                .fillMaxHeight()
                .padding(top = 10.dp, bottom = 25.dp),
            verticalArrangement = Arrangement.SpaceBetween
        ) {
            // Distance values corresponding to RSSI values from lookup table
            Text("0.14m", fontSize = 10.sp, color = Color(0xFF4CAF50))  // -20 dBm
            Text("0.35m", fontSize = 10.sp, color = Color(0xFF4CAF50))  // -30 dBm
            Text("0.91m", fontSize = 10.sp, color = Color(0xFF4CAF50))  // -40 dBm
            Text("2.35m", fontSize = 10.sp, color = Color(0xFF4CAF50))  // -50 dBm
            Text("6.07m", fontSize = 10.sp, color = Color(0xFF4CAF50))  // -60 dBm
            Text("15.7m", fontSize = 10.sp, color = Color(0xFF4CAF50))  // -70 dBm
            Text("40.6m", fontSize = 10.sp, color = Color(0xFF4CAF50))  // -80 dBm
            Text("105m", fontSize = 10.sp, color = Color(0xFF4CAF50))   // -90 dBm
            Text("271m", fontSize = 10.sp, color = Color(0xFF4CAF50))   // -100 dBm
        }
        
        // Graph canvas
        Canvas(
            modifier = Modifier
                .fillMaxSize()
                .padding(start = 50.dp, end = 55.dp, bottom = 25.dp, top = 10.dp)
        ) {
            drawDualAxisGraph(rssiHistory, distanceHistory)
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
        
        // Removed overlapping text labels and legend
    }
}

private fun DrawScope.drawDualAxisGraph(
    rssiHistory: List<RssiData>,
    distanceHistory: List<DistancePoint>
) {
    val width = size.width
    val height = size.height
    
    // Comprehensive debug logging
    android.util.Log.d("GraphDebug", "========== GRAPH PLOTTING DEBUG ==========")
    android.util.Log.d("GraphDebug", "Canvas dimensions: ${width}x${height}")
    android.util.Log.d("GraphDebug", "RSSI History size: ${rssiHistory.size}")
    android.util.Log.d("GraphDebug", "Distance History size: ${distanceHistory.size}")
    
    if (rssiHistory.isNotEmpty()) {
        val rssiValues = rssiHistory.map { it.value }
        android.util.Log.d("GraphDebug", "RSSI Range: ${rssiValues.minOrNull()} to ${rssiValues.maxOrNull()} dBm")
        android.util.Log.d("GraphDebug", "Last 5 RSSI values: ${rssiHistory.takeLast(5).map { "${it.value}dBm" }.joinToString()}")
    }
    
    if (distanceHistory.isNotEmpty()) {
        val distances = distanceHistory.map { it.distance }
        android.util.Log.d("GraphDebug", "Distance Range: ${distances.minOrNull()}m to ${distances.maxOrNull()}m")
        android.util.Log.d("GraphDebug", "All distance points:")
        distanceHistory.forEachIndexed { index, point ->
            android.util.Log.d("GraphDebug", "  [$index] Distance: ${point.distance}m, RSSI: ${point.rssiValue}dBm, Time: ${point.timestamp}")
        }
    } else {
        android.util.Log.w("GraphDebug", "⚠️ NO DISTANCE POINTS TO PLOT!")
    }
    
    // Draw grid lines
    val gridColor = Color.LightGray.copy(alpha = 0.3f)
    
    // Horizontal grid lines (9 lines for RSSI/distance values)
    for (i in 0..8) {
        val y = height * i / 8
        drawLine(
            color = gridColor,
            start = Offset(0f, y),
            end = Offset(width, y),
            strokeWidth = 0.5.dp.toPx()
        )
    }
    
    // Vertical grid lines (time - 6 lines for 30 seconds)
    for (i in 0..6) {
        val x = width * i / 6
        drawLine(
            color = gridColor,
            start = Offset(x, 0f),
            end = Offset(x, height),
            strokeWidth = 0.5.dp.toPx()
        )
    }
    
    // RSSI range for normalization
    val minRssi = -100f
    val maxRssi = -20f
    val rssiRange = maxRssi - minRssi
    
    // Distance range for normalization (logarithmic scale)
    val minDistanceLog = kotlin.math.log10(0.14f)  // log10(0.14m) for -20 dBm
    val maxDistanceLog = kotlin.math.log10(271f)    // log10(271m) for -100 dBm
    val distanceLogRange = maxDistanceLog - minDistanceLog
    
    // Draw RSSI line (Blue)
    if (rssiHistory.size > 1) {
        android.util.Log.d("GraphDebug", "--- Drawing RSSI Line (Blue) ---")
        val rssiPath = Path()
        var rssiPointCount = 0
        
        rssiHistory.forEachIndexed { index, rssiData ->
            val x = (index.toFloat() / 299f) * width // Max 300 points
            val normalizedRssi = (rssiData.value - minRssi) / rssiRange
            val y = height * (1f - normalizedRssi)
            
            if (index < 3 || index >= rssiHistory.size - 3) {
                android.util.Log.d("GraphDebug", "RSSI Point[$index]: value=${rssiData.value}dBm, x=$x, y=$y, normalized=$normalizedRssi")
            }
            
            if (index == 0) {
                rssiPath.moveTo(x, y)
            } else {
                rssiPath.lineTo(x, y)
            }
            rssiPointCount++
        }
        android.util.Log.d("GraphDebug", "Drew $rssiPointCount RSSI points")
        
        // Draw the RSSI line
        drawPath(
            path = rssiPath,
            color = Color.Blue,
            style = Stroke(width = 2.dp.toPx())
        )
        
        // Draw dots for recent RSSI points
        val recentRssiPoints = rssiHistory.takeLast(10)
        recentRssiPoints.forEachIndexed { index, rssiData ->
            val globalIndex = rssiHistory.size - recentRssiPoints.size + index
            val x = (globalIndex.toFloat() / 299f) * width
            val normalizedRssi = (rssiData.value - minRssi) / rssiRange
            val y = height * (1f - normalizedRssi)
            
            drawCircle(
                color = Color.Blue,
                radius = 2.dp.toPx(),
                center = Offset(x, y)
            )
        }
    }
    
    // Draw Distance line (Green) - Stage 2 output
    android.util.Log.d("GraphDebug", "--- Drawing Distance Line (Green) ---")
    
    if (distanceHistory.size >= 1) {  // Changed from > 1 to >= 1 to show even single points
        val distancePath = Path()
        var distancePointCount = 0
        
        distanceHistory.forEachIndexed { index, distancePoint ->
            // Calculate x position based on timestamp relative to RSSI history
            val x = if (rssiHistory.isNotEmpty()) {
                val firstRssiTime = rssiHistory.first().timestamp
                val lastRssiTime = rssiHistory.last().timestamp
                val timeRange = lastRssiTime - firstRssiTime
                if (timeRange > 0) {
                    val relativeTime = (distancePoint.timestamp - firstRssiTime).toFloat() / timeRange
                    relativeTime * width
                } else {
                    // If only one point or no time range, place it at the right edge
                    width * 0.9f
                }
            } else {
                // If no RSSI history, distribute evenly
                if (distanceHistory.size == 1) {
                    width * 0.9f
                } else {
                    (index.toFloat() / (distanceHistory.size - 1)) * width
                }
            }
            
            // Use logarithmic scale for distance to match the non-linear RSSI-distance relationship
            val logDistance = kotlin.math.log10(distancePoint.distance.coerceAtLeast(0.1f))
            val normalizedDistance = (logDistance - minDistanceLog) / distanceLogRange
            val y = height * (1f - normalizedDistance)
            
            android.util.Log.d("GraphDebug", "Distance Point[$index]: distance=${distancePoint.distance}m, " +
                "x=$x, y=$y, logDist=$logDistance, normalized=$normalizedDistance")
            
            if (index == 0) {
                distancePath.moveTo(x, y)
            } else {
                distancePath.lineTo(x, y)
            }
            distancePointCount++
        }
        android.util.Log.d("GraphDebug", "Drew $distancePointCount distance points")
        
        // Draw the distance line
        drawPath(
            path = distancePath,
            color = Color(0xFF4CAF50),
            style = Stroke(width = 2.5.dp.toPx())
        )
        
        // Draw dots for distance points
        distanceHistory.takeLast(10).forEach { distancePoint ->
            val x = if (rssiHistory.isNotEmpty()) {
                val firstRssiTime = rssiHistory.first().timestamp
                val lastRssiTime = rssiHistory.last().timestamp
                val timeRange = lastRssiTime - firstRssiTime
                if (timeRange > 0) {
                    val relativeTime = (distancePoint.timestamp - firstRssiTime).toFloat() / timeRange
                    relativeTime * width
                } else {
                    width / 2
                }
            } else {
                width / 2
            }
            
            val logDistance = kotlin.math.log10(distancePoint.distance.coerceAtLeast(0.1f))
            val normalizedDistance = (logDistance - minDistanceLog) / distanceLogRange
            val y = height * (1f - normalizedDistance)
            
            drawCircle(
                color = Color(0xFF4CAF50),
                radius = 3.dp.toPx(),
                center = Offset(x, y)
            )
        }
    } else {
        android.util.Log.w("GraphDebug", "⚠️ NOT DRAWING DISTANCE LINE - No points available!")
    }
    
    android.util.Log.d("GraphDebug", "========== END GRAPH DEBUG ==========")
}
