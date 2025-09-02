package com.singleping.motoapp.export

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import com.google.android.gms.auth.api.signin.GoogleSignIn
import com.google.android.gms.auth.api.signin.GoogleSignInAccount
import com.google.android.gms.auth.api.signin.GoogleSignInClient
import com.google.android.gms.auth.api.signin.GoogleSignInOptions
import com.google.android.gms.common.api.Scope
import com.google.api.client.http.javanet.NetHttpTransport
import com.google.api.client.json.gson.GsonFactory
import com.google.api.services.drive.Drive
import com.google.api.services.drive.DriveScopes
import com.google.api.services.drive.model.File
import com.singleping.motoapp.data.LogData
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.ByteArrayOutputStream
import java.text.SimpleDateFormat
import java.util.*

class DataExporter(private val context: Context) {
    
    companion object {
        private const val TAG = "DataExporter"
        private const val REQUEST_SIGN_IN = 1001
    }
    
    private var googleSignInClient: GoogleSignInClient? = null
    private var driveService: Drive? = null
    private var signInLauncher: ActivityResultLauncher<Intent>? = null
    private var onExportComplete: ((Boolean, String) -> Unit)? = null
    
    /**
     * Initialize the exporter with an activity for sign-in
     */
    fun initialize(activity: ComponentActivity, onComplete: (Boolean, String) -> Unit) {
        onExportComplete = onComplete
        
        // Configure Google Sign-In
        val signInOptions = GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_SIGN_IN)
            .requestEmail()
            .requestScopes(Scope(DriveScopes.DRIVE_FILE))
            .build()
        
        googleSignInClient = GoogleSignIn.getClient(activity, signInOptions)
        
        // Register activity result launcher for sign-in
        signInLauncher = activity.registerForActivityResult(
            ActivityResultContracts.StartActivityForResult()
        ) { result ->
            if (result.resultCode == Activity.RESULT_OK) {
                handleSignInResult(result.data)
            } else {
                onExportComplete?.invoke(false, "Sign-in cancelled")
            }
        }
    }
    
    /**
     * Export log data to Google Drive
     */
    suspend fun exportToGoogleDrive(logData: List<LogData>) {
        withContext(Dispatchers.IO) {
            try {
                // Check if already signed in
                val account = GoogleSignIn.getLastSignedInAccount(context)
                if (account != null && account.grantedScopes.contains(Scope(DriveScopes.DRIVE_FILE))) {
                    // Already signed in with required permissions
                    setupDriveService(account)
                    uploadToDrive(logData)
                } else {
                    // Need to sign in
                    withContext(Dispatchers.Main) {
                        signIn()
                    }
                }
            } catch (e: Exception) {
                Log.e(TAG, "Export failed", e)
                withContext(Dispatchers.Main) {
                    onExportComplete?.invoke(false, "Export failed: ${e.message}")
                }
            }
        }
    }
    
    /**
     * Start Google Sign-In flow
     */
    private fun signIn() {
        val signInIntent = googleSignInClient?.signInIntent
        signInIntent?.let { intent ->
            signInLauncher?.launch(intent)
        } ?: run {
            onExportComplete?.invoke(false, "Failed to create sign-in intent")
        }
    }
    
    /**
     * Handle sign-in result
     */
    private fun handleSignInResult(data: Intent?) {
        GoogleSignIn.getSignedInAccountFromIntent(data)
            .addOnSuccessListener { account ->
                setupDriveService(account)
                // Continue with export after successful sign-in
                GlobalScope.launch {
                    val logData = getCurrentLogData()
                    uploadToDrive(logData)
                }
            }
            .addOnFailureListener { e ->
                Log.e(TAG, "Sign-in failed", e)
                onExportComplete?.invoke(false, "Sign-in failed: ${e.message}")
            }
    }
    
    /**
     * Set up Google Drive service
     */
    private fun setupDriveService(account: GoogleSignInAccount) {
        // Build Drive service using account token
        val httpTransport = NetHttpTransport()
        val jsonFactory = GsonFactory.getDefaultInstance()
        
        // Get the account token
        val accessToken = account.idToken ?: ""
        
        driveService = Drive.Builder(
            httpTransport,
            jsonFactory,
            { request ->
                request.headers.authorization = "Bearer $accessToken"
            }
        )
            .setApplicationName("SinglePing MotoApp")
            .build()
    }
    
    /**
     * Upload CSV data to Google Drive
     */
    private suspend fun uploadToDrive(logData: List<LogData>) {
        withContext(Dispatchers.IO) {
            try {
                // Generate CSV content
                val csvContent = generateCsvContent(logData)
                
                // Create file metadata
                val timestamp = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())
                val fileMetadata = File().apply {
                    name = "SinglePing_RSSI_Log_$timestamp.csv"
                    mimeType = "text/csv"
                    description = "RSSI log data from SinglePing MotoApp"
                }
                
                // Create file content
                val contentStream = ByteArrayOutputStream().apply {
                    write(csvContent.toByteArray())
                }
                
                val mediaContent = com.google.api.client.http.ByteArrayContent(
                    "text/csv",
                    contentStream.toByteArray()
                )
                
                // Upload file to Google Drive
                val file = driveService?.files()?.create(fileMetadata, mediaContent)
                    ?.setFields("id, name, webViewLink")
                    ?.execute()
                
                if (file != null) {
                    Log.i(TAG, "File uploaded successfully: ${file.name}")
                    withContext(Dispatchers.Main) {
                        onExportComplete?.invoke(
                            true,
                            "File uploaded to Google Drive: ${file.name}"
                        )
                    }
                } else {
                    throw Exception("Failed to upload file")
                }
                
            } catch (e: Exception) {
                Log.e(TAG, "Upload failed", e)
                withContext(Dispatchers.Main) {
                    onExportComplete?.invoke(false, "Upload failed: ${e.message}")
                }
            }
        }
    }
    
    /**
     * Generate CSV content from log data
     */
    private fun generateCsvContent(logData: List<LogData>): String {
        val csv = StringBuilder()
        
        // Add header row
        csv.append("Timestamp,RSSI (dBm),Distance (m),Host Device,Host Battery,")
        csv.append("Mipe Status,Mipe RSSI,Mipe Address,Mipe Battery (V)\n")
        
        // Add data rows
        val dateFormat = SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.US)
        
        logData.forEach { log ->
            csv.append("${dateFormat.format(Date(log.timestamp))},")
            csv.append("${log.rssi},")
            csv.append("${String.format(Locale.US, "%.2f", log.distance)},")
            csv.append("${log.hostInfo.deviceName},")
            csv.append("${log.hostInfo.batteryLevel},")
            csv.append("${log.mipeStatus?.connectionState ?: "N/A"},")
            csv.append("${log.mipeStatus?.rssi ?: "N/A"},")
            csv.append("${log.mipeStatus?.deviceAddress ?: "N/A"},")
            csv.append("${log.mipeStatus?.batteryVoltage?.let { String.format(Locale.US, "%.2f", it) } ?: "N/A"}\n")
        }
        
        return csv.toString()
    }
    
    /**
     * Get current log data (placeholder - should be provided by caller)
     */
    private fun getCurrentLogData(): List<LogData> {
        // This should be provided by the caller
        return emptyList()
    }
    
    /**
     * Sign out from Google account
     */
    fun signOut() {
        googleSignInClient?.signOut()
        driveService = null
    }
}
