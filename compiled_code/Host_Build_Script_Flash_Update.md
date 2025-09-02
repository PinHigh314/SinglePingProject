# Host Build Script - Flash Functionality Added
Date: September 2, 2025

## Overview
Added interactive flashing prompt to the Host build script, matching the functionality in the Mipe build script.

## Changes Made

### build_host.bat Updates:
1. **Added Flash Prompt Section**:
   - Interactive Y/N prompt after successful build
   - Asks: "Do you want to flash the Host device now? (Y/N):"

2. **Flash Execution**:
   - If user selects Y: Runs `nrfjprog --program build\zephyr\zephyr.hex --chiperase --verify -r`
   - Provides success/failure feedback
   - Shows helpful error messages if flashing fails

3. **Error Handling**:
   - Checks nrfjprog return code
   - Provides troubleshooting steps if flash fails:
     - Device connection check
     - nrfjprog installation verification
     - Power status confirmation

4. **Skip Option**:
   - If user selects N: Shows manual flash command for later use
   - Preserves the hex file for manual flashing

## Usage

### Build and Flash:
```bash
cd "BAT files"
./build_host.bat

# When prompted:
Do you want to flash the Host device now? (Y/N): Y
```

### Build Only (No Flash):
```bash
cd "BAT files"
./build_host.bat

# When prompted:
Do you want to flash the Host device now? (Y/N): N
```

### Clean Build with Flash:
```bash
./build_host.bat --clean

# Then respond Y to flash prompt
```

## Benefits

1. **Streamlined Workflow**: Build and flash in one command
2. **User Control**: Optional flashing preserves flexibility
3. **Consistent Experience**: Matches Mipe script behavior
4. **Error Recovery**: Clear feedback if flashing fails
5. **Manual Option**: Always shows manual flash command

## Requirements

- nrfjprog must be installed and in PATH
- Host device must be connected via USB
- Device must be powered on

## Manual Flash Command
If you skip the automatic flash or need to flash later:
```bash
nrfjprog --program build\zephyr\zephyr.hex --chiperase --verify -r
```

## Notes
- The script still copies the hex file to `compiled_code/` directory with timestamp
- Version logging continues to work as before
- All existing functionality is preserved
