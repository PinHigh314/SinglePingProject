# Universal Build Instructions for Host Device Test Firmware

## Why the Build Process Was Complicated

You're absolutely right - the build process was unnecessarily complex because:
1. I was creating a new build script for each version (v1, v2, v3, etc.)
2. I was trying different approaches instead of sticking to one proven method
3. I wasn't reusing the successful pattern from v5

## The Simple Solution: Universal Build Script

Now we have `build_test_universal.ps1` that can build ANY test version with a simple command.

## How to Use It

### For any test version:
```powershell
# Build v6 (alternating RSSI)
.\build_test_universal.ps1 -Version 6 -Description "alternating rssi" -RevNumber 019

# Build v5 (fixed LED)
.\build_test_universal.ps1 -Version 5 -Description "fixed led" -RevNumber 018

# Build v7 (when you create it)
.\build_test_universal.ps1 -Version 7 -Description "your new feature" -RevNumber 020
```

## The Process is Now:
1. Create your test source file: `src/main_test_fixed_rssi_vX.c`
2. Run the universal build script with version number and description
3. Done! The hex file is automatically copied to compiled_code with proper naming

## What the Universal Script Does:
1. Sets up the build environment (same every time)
2. Backs up current files
3. Copies your test version to main.c
4. Copies test BLE central (for test builds)
5. Runs cmake and ninja build
6. Copies output hex to compiled_code with descriptive name
7. Restores original files

## No More:
- Creating new build scripts for each version
- Complex cmake manipulations
- Manual file copying
- Confusion about which build method to use

The script handles everything automatically and consistently!
