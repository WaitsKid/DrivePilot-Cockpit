$ErrorActionPreference = "Stop"

Write-Host "Checking DrivePilot local services..."

try {
    $dms = Invoke-RestMethod -Uri "http://127.0.0.1:8765/health" -TimeoutSec 3
    Write-Host "DMS:" ($dms | ConvertTo-Json -Compress)
} catch {
    Write-Warning "DMS unavailable: $($_.Exception.Message)"
}

try {
    $agent = Invoke-RestMethod -Uri "http://127.0.0.1:8770/health" -TimeoutSec 3
    Write-Host "Agent:" ($agent | ConvertTo-Json -Compress)
} catch {
    Write-Warning "Agent unavailable: $($_.Exception.Message)"
}
