& {
    $buildDir = "build"
    
    Write-Host "Building project..." -ForegroundColor Cyan
    cmake --build $buildDir

    $exePath = Join-Path $buildDir "Debug\VulkanEngine.exe"

    if (Test-Path $exePath) {
        Write-Host "Running VulkanEngine..." -ForegroundColor Green
        & $exePath
    } else {
        Write-Host "Error: Executable not found at $exePath" -ForegroundColor Red
    }
}