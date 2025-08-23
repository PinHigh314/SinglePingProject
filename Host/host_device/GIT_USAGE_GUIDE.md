# Git Usage Guide for Host Device

## Current State
- The project now uses Git for version control
- All versioned files (v1-v9) have been cleaned up
- Current code includes the LED1 timing fix from v9
- Single build script: `build.ps1`

## Basic Git Workflow

### 1. Check Current Status
```bash
git status
git log --oneline -5  # Show last 5 commits
```

### 2. Make Changes
Edit files as needed (main.c, ble files, etc.)

### 3. Commit Changes
```bash
git add .
git commit -m "Description of changes"
```

### 4. Build and Test
```bash
cd Host/host_device
./build.ps1
```

### 5. Push to GitHub (if build successful)
```bash
git push
```

## Going Back to Previous Versions

### View Available Tags
```bash
git tag -l
```

Current tags:
- `backup-before-cleanup` - All versioned files before cleanup
- `v1.0-led-fix` - Clean version with LED1 timing fix

### Checkout a Specific Tag
```bash
git checkout v1.0-led-fix  # Go to specific version
git checkout main          # Return to latest
```

### Revert Changes (if something goes wrong)
```bash
git reset --hard HEAD~1    # Undo last commit
git checkout -- .          # Discard uncommitted changes
```

## Best Practices

1. **Commit Often**: Create save points before major changes
2. **Clear Messages**: Use descriptive commit messages
3. **Test Before Push**: Always build and test before pushing
4. **Tag Milestones**: Create tags for working versions

## Example Workflow

```bash
# Start work
git status

# Make LED changes
# Edit src/main.c

# Commit
git add .
git commit -m "Fix LED2 flash duration for real RSSI"

# Build
./build.ps1

# If successful
git push
git tag -a "v1.1-led2-fix" -m "Fixed LED2 flash duration"

# If failed
git reset --hard HEAD~1  # Go back
```

## Project Structure

```
Host/host_device/
├── src/
│   ├── main.c              # Main application (with LED fix)
│   └── ble/
│       ├── ble_central.c   # BLE central (Mipe connection)
│       ├── ble_peripheral.c # BLE peripheral (MotoApp connection)
│       └── ble_peripheral.h # Header file
├── build.ps1               # Build script
├── CMakeLists.txt          # CMake configuration
└── prj.conf                # Project configuration
```

## LED Status Reference

- **LED0**: Heartbeat (500ms blink)
- **LED1**: MotoApp connection
  - OFF: No connection
  - Solid ON: MotoApp only
  - Rapid flash (100ms): Both MotoApp and Mipe
- **LED2**: Real RSSI transmission flash
- **LED3**: Fixed RSSI (-55) transmission flash + streaming active
