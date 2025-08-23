# Git Version Control Strategy for SinglePing Project

## Overview
This document outlines the version control strategy for tracking firmware builds, app releases, and major milestones.

## Branching Strategy

### Main Branches
- `main` - Stable, tested releases only
- `develop` - Integration branch for ongoing development
- `feature/*` - Feature development branches
- `test/*` - Test/debug branches (like our current RSSI testing)
- `release/*` - Release preparation branches

### Current Branch Structure Proposal
```
main
├── develop
│   ├── feature/motoapp-ble
│   ├── feature/host-firmware
│   ├── test/fixed-rssi-debug
│   └── test/led-behavior-fix
```

## Tagging Strategy

### Tag Format
- Firmware: `firmware/host-v{version}-rev{revision}`
- Apps: `app/motoapp-v{version}`
- Milestones: `milestone/{description}-{date}`

### Examples
- `firmware/host-v1.0-rev018`
- `app/motoapp-v3.5`
- `milestone/ble-data-flow-working-20250823`

## Commit Message Convention

### Format
```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types
- `feat`: New feature
- `fix`: Bug fix
- `test`: Test code
- `docs`: Documentation
- `build`: Build system changes
- `refactor`: Code refactoring

### Examples
```
fix(host): LED2 no longer activates on MotoApp connection

- Removed incorrect mipe_connected simulation
- LED2 now reserved for actual Mipe connections only
- Fixes issue #14

test(host): Add fixed RSSI test firmware v5

- Implements proper command-based streaming trigger
- Fixed RSSI value of -55 dBm for testing
- Build: rev018
```

## Implementation Commands

### 1. Create Current Milestone Tag
```bash
# Tag the current working state
git add -A
git commit -m "milestone: BLE data flow established between Host and MotoApp

- Host firmware v5 (rev018) with fixed LED behavior
- MotoApp v3.5 with full BLE support
- Fixed RSSI test data flowing successfully
- Proper command-based streaming control working"

git tag -a milestone/ble-data-flow-working-20250823 -m "BLE data flow milestone: Host v5 test firmware successfully streaming to MotoApp v3.5"
```

### 2. Create Feature Branch for Current Work
```bash
# Create test branch for our debugging work
git checkout -b test/fixed-rssi-debug
git add Host/host_device/src/main_test_fixed_rssi_v*.c
git add Host/host_device/src/ble/ble_central_test.c
git add Host/host_device/build_test_fixed_rssi_v*.ps1
git commit -m "test(host): Fixed RSSI test implementations v1-v5"
```

### 3. Tag Specific Builds
```bash
# Tag the successful v5 build
git tag -a firmware/host-test-v5-rev018 -m "Test firmware v5: Fixed LED behavior and command-based streaming"

# Tag MotoApp version
git tag -a app/motoapp-v3.5 -m "MotoApp v3.5: Full BLE support with streaming control"
```

### 4. Create Release Branch When Ready
```bash
# When ready to prepare for production
git checkout -b release/1.0
# Apply final fixes, update version numbers
# Then merge to main and tag
```

## Backup Strategy

### Local Backups
1. Keep test versions in `compiled_code/` (as currently done)
2. Document each build in `compiled_code/version_log.md`
3. Maintain README files for significant versions

### Remote Backups
1. Push all tags to GitHub
2. Create GitHub releases for major milestones
3. Attach compiled binaries to releases

## Quick Reference Commands

```bash
# View all tags
git tag -l

# View firmware tags only
git tag -l "firmware/*"

# Checkout specific tagged version
git checkout tags/firmware/host-v5-rev018

# Push tags to remote
git push origin --tags

# Create annotated tag for current state
git tag -a <tagname> -m "<description>"
```

## Current State Snapshot

As of 2025-08-23:
- **Working Firmware**: host_device_test_fixed_led_v5_20250823_rev018.hex
- **Working App**: MotoApp v3.5
- **Status**: BLE data flow established, LED behavior fixed
- **Test Mode**: Fixed RSSI -55 dBm streaming successfully
