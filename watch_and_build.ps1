$watcher = New-Object System.IO.FileSystemWatcher
$watcher.Path = "$PSScriptRoot\src"
$watcher.IncludeSubdirectories = $true
$watcher.EnableRaisingEvents = $true

$action = {
    $path = $Event.SourceEventArgs.FullPath
    $changeType = $Event.SourceEventArgs.ChangeType
    if ($path -match '\.(cpp|hpp)$') {
        Write-Host "`n[$([DateTime]::Now.ToString('HH:mm:ss'))] File changed: $path ($changeType)" -ForegroundColor Cyan
        Write-Host "Building project..." -ForegroundColor Yellow
        
        # Adjust this build command if you are using a different build system
        cmake --build "$PSScriptRoot\build"
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Build Successful!" -ForegroundColor Green
        } else {
            Write-Host "Build Failed." -ForegroundColor Red
        }
    }
}

Register-ObjectEvent $watcher "Changed" -Action $action > $null
Register-ObjectEvent $watcher "Created" -Action $action > $null
Register-ObjectEvent $watcher "Deleted" -Action $action > $null
Register-ObjectEvent $watcher "Renamed" -Action $action > $null

Write-Host "Watching for C++ file changes in src/ directory..." -ForegroundColor Green
Write-Host "Press Ctrl+C to stop." -ForegroundColor Gray

try {
    while ($true) { Start-Sleep -Seconds 1 }
} finally {
    Unregister-Event -SourceIdentifier $watcher.Name -ErrorAction SilentlyContinue
    $watcher.Dispose()
}
