package com.singleping.motoapp.data

data class MipeStatus(
    val connectionState: String,
    val rssi: Int,
    val deviceName: String?,
    val deviceAddress: String?,
    val connectionDuration: Long,
    val lastSeen: Long
)
