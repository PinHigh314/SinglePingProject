# MotoApp v3.4 - BLE Permission Fix

## Version Information
- **Version:** 3.4
- **Date:** August 23, 2025
- **File:** MotoApp_TMT1_v3.4_Permission_Fix.apk
- **Size:** 15.6 MB

## Critical Issue Fixed
The MotoApp was unable to detect ANY BLE devices, including the Host device (MIPE_HOST_A1B2). The root cause was **missing location permissions** for BLE scanning.

## Root Cause Analysis

### The Permission Problem
Android requires location permissions for BLE scanning due to privacy concerns (BLE can be used for location tracking). The app had several critical permission issues:

1. **AndroidManifest.xml Issues:**
   - `ACCESS_FINE_LOCATION` had `maxSdkVersion="30"` - not available for Android 12+ (API 31+)
   - Missing `ACCESS_COARSE_LOCATION` as a fallback
   - Location permission is STILL required for BLE scanning on Android 12+, in addition to BLUETOOTH_SCAN

2. **MainActivity.kt Issues:**
   - Only requested `BLUETOOTH_SCAN` and `BLUETOOTH_CONNECT` for Android 12+
   - **Missing location permission request for Android 12+**
   - This prevented the scanner from seeing ANY devices

### Android BLE Permission Requirements
- **Android 6-11 (API 23-30):** Requires `ACCESS_FINE_LOCATION` or `ACCESS_COARSE_LOCATION`
- **Android 12+ (API 31+):** Requires:
  - `BLUETOOTH_SCAN`
  - `BLUETOOTH_CONNECT`
  - **PLUS** either `ACCESS_FINE_LOCATION` or `ACCESS_COARSE_LOCATION`

## Changes Made

### 1. AndroidManifest.xml
```xml
<!-- Location permissions for BLE scanning (required for all Android versions) -->
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
<uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION" />

<!-- Bluetooth permissions for Android 12+ -->
<uses-permission android:name="android.permission.BLUETOOTH_SCAN" />
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />
<uses-permission android:name="android.permission.BLUETOOTH_ADVERTISE" />
```

### 2. MainActivity.kt
Added location permission to Android 12+ runtime permission requests:
```kotlin
val requiredPermissions = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
    arrayOf(
        Manifest.permission.BLUETOOTH_SCAN,
        Manifest.permission.BLUETOOTH_CONNECT,
        Manifest.permission.ACCESS_FINE_LOCATION  // Added this!
    )
}
```

### 3. Enhanced DebugScreen.kt
- Added permission status display showing all required permissions
- Shows Bluetooth enabled/disabled status
- Enhanced scanner with service UUID detection
- Better error messages for missing permissions
- Parses and displays BLE service UUIDs from scan records
- Highlights devices with SinglePing service UUID (12345678-1234-5678-1234-56789abcdef0)

## Features in v3.4

### Debug Scanner Enhancements
1. **Permission Status Card** - Shows status of all required permissions:
   - Bluetooth Scan permission
   - Bluetooth Connect permission
   - Location permission
   - Bluetooth enabled status

2. **Service UUID Detection** - Parses advertising data to find service UUIDs
   - Highlights devices advertising SinglePing service
   - Shows total number of services per device

3. **Improved Visual Feedback**:
   - Green background for devices with correct name (MIPE_HOST_A1B2)
   - Blue background for devices with SinglePing service UUID
   - Purple background for potential matches (MIPE, HOST, nRF in name)

4. **Better Logging** - Comprehensive logging for debugging:
   - Shows every device found with name, address, RSSI, and UUIDs
   - Logs permission status
   - Tracks scan callback count

## Testing Instructions

1. **Install the APK** on your Android device
2. **Launch the app** - it will immediately request permissions
3. **Grant all permissions** when prompted:
   - Bluetooth permissions (Android 12+)
   - Location permission (required for BLE scanning)
4. **Ensure Bluetooth is enabled** on your device
5. **Open the Debug Scanner** (app opens directly to it)
6. **Check Permission Status Card** - all should show ✅
7. **Press "Start Scan"** to begin scanning
8. **Look for MIPE_HOST_A1B2** in the device list

## Expected Results

With the Host device advertising:
- The scanner should find "MIPE_HOST_A1B2"
- The device should be highlighted in green
- If the Host is advertising the service UUID, it will show "✅ SinglePing Service"
- RSSI values should be displayed (typically -40 to -80 dBm)

## Troubleshooting

If the scanner still doesn't find devices:
1. **Check Permission Status Card** - all items should show ✅
2. **Ensure Bluetooth is enabled** - check the status display
3. **Verify Host is advertising** - use nRF Connect app to confirm
4. **Check Android version** - different versions have different requirements
5. **Try force-closing and reopening** the app after granting permissions
6. **Check location services** - must be enabled on the device

## Known Issues from Logs

From the version logs, the Host firmware has a known issue where BLE Central scanning (for MIPE devices) can interfere with Peripheral advertising (to MotoApp). The fix was implemented in `host_device_tmt3_scanning_fix_20250821_rev008.hex` which delays MIPE scanning until after MotoApp connects.

## Next Steps

Once BLE scanning is working:
1. Test connection to Host device
2. Verify GATT service discovery
3. Test RSSI data streaming
4. Implement automatic reconnection
5. Add background scanning service

## Technical Notes

- Uses Nordic Android BLE Library v2.7.0
- Implements Nordic's BluetoothLeScannerCompat for backward compatibility
- Scan mode set to LOW_LATENCY for best performance
- No UUID filtering to ensure all devices are visible
- Service UUID: 12345678-1234-5678-1234-56789abcdef0
- Host device name: MIPE_HOST_A1B2
