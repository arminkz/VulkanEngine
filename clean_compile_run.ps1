& {
    $buildDir = "build"

    # Remove the build directory if it exists
    if (Test-Path $buildDir) {
        Write-Host "Removing existing build directory..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force $buildDir
    }

    # Recreate the build directory
    Write-Host "Creating new build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $buildDir | Out-Null

    # Run CMake to generate build system
    Write-Host "Running CMake..." -ForegroundColor Cyan
    cmake -S . -B build

    # Build the project
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