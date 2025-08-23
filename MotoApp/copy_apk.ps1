# Script to copy the built APK to the compiled_code directory
$sourcePath = "app\build\outputs\apk\debug\app-debug.apk"
$destPath = "..\compiled_code\MotoApp_TMT1_v2.0.apk"

if (Test-Path $sourcePath) {
    Copy-Item -Path $sourcePath -Destination $destPath -Force
    Write-Host "APK copied successfully to: $destPath"
    Write-Host "File size: $((Get-Item $destPath).Length / 1MB) MB"
} else {
    Write-Host "APK not found at: $sourcePath"
    Write-Host "Please ensure the build completed successfully."
}
