# PowerShell script to set up git versioning for the current milestone
# Run this to create proper version control for the current working state

Write-Host "=== Setting up Git Version Control ===" -ForegroundColor Cyan

# Check if we're in a git repository
if (!(Test-Path .git)) {
    Write-Host "ERROR: Not in a git repository!" -ForegroundColor Red
    exit 1
}

# Function to create commit with all current changes
function Create-MilestoneCommit {
    Write-Host "`nCreating milestone commit..." -ForegroundColor Yellow
    
    # Stage all changes
    git add -A
    
    # Create milestone commit
    $commitMessage = @"
milestone: BLE data flow established between Host and MotoApp

- Host firmware v5 (rev018) with fixed LED behavior
- MotoApp v3.5 with full BLE support
- Fixed RSSI test data flowing successfully
- Proper command-based streaming control working
"@
    
    git commit -m $commitMessage
    Write-Host "Milestone commit created!" -ForegroundColor Green
}

# Function to create tags
function Create-Tags {
    Write-Host "`nCreating tags..." -ForegroundColor Yellow
    
    # Milestone tag
    git tag -a "milestone/ble-data-flow-working-20250823" -m "BLE data flow milestone: Host v5 test firmware successfully streaming to MotoApp v3.5"
    Write-Host "Created: milestone/ble-data-flow-working-20250823" -ForegroundColor Green
    
    # Firmware tag
    git tag -a "firmware/host-test-v5-rev018" -m "Test firmware v5: Fixed LED behavior and command-based streaming"
    Write-Host "Created: firmware/host-test-v5-rev018" -ForegroundColor Green
    
    # App tag
    git tag -a "app/motoapp-v3.5" -m "MotoApp v3.5: Full BLE support with streaming control"
    Write-Host "Created: app/motoapp-v3.5" -ForegroundColor Green
}

# Function to create test branch
function Create-TestBranch {
    Write-Host "`nCreating test branch..." -ForegroundColor Yellow
    
    # Create and checkout test branch
    git checkout -b test/fixed-rssi-debug
    Write-Host "Created and switched to: test/fixed-rssi-debug" -ForegroundColor Green
}

# Main execution
Write-Host "`nThis script will:"
Write-Host "1. Commit all current changes as a milestone"
Write-Host "2. Create tags for the milestone, firmware, and app"
Write-Host "3. Create a test branch for the RSSI debugging work"
Write-Host "`nCurrent status:" -ForegroundColor Yellow
git status --short

$response = Read-Host "`nDo you want to proceed? (Y/N)"
if ($response -eq 'Y' -or $response -eq 'y') {
    Create-MilestoneCommit
    Create-Tags
    Create-TestBranch
    
    Write-Host "`n=== Git Versioning Setup Complete ===" -ForegroundColor Green
    Write-Host "`nNext steps:"
    Write-Host "1. Push changes and tags: git push origin --all --tags"
    Write-Host "2. Continue development on test/fixed-rssi-debug branch"
    Write-Host "3. When ready for production, create a release branch"
    
    Write-Host "`nView all tags: git tag -l"
    Write-Host "View current branch: git branch --show-current"
} else {
    Write-Host "Operation cancelled." -ForegroundColor Yellow
}
