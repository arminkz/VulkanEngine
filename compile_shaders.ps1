# C:/VulkanSDK/1.4.309.0/Bin/glslc.exe shaders/shader.vert -o spv/vert.spv
# C:/VulkanSDK/1.4.309.0/Bin/glslc.exe shaders/shader.frag -o spv/frag.spv

# This script compiles GLSL shaders to SPIR-V using glslc.exe
$shaderFolder = "shaders"

# Set the path to glslc.exe if it's not in PATH
$glslcPath = "C:\VulkanSDK\1.4.309.0\Bin\glslc.exe"  # or "glslc"

# Create output folder if it doesn't exist
$outputFolder = "spv"
if (-not (Test-Path $outputFolder)) {
    New-Item -ItemType Directory -Path $outputFolder
}

# Go through each shader file
Get-ChildItem -Path $shaderFolder -File | ForEach-Object {
    $inputFile = $_.FullName
    $outputFile = Join-Path $outputFolder ($_.BaseName + "_" + $_.Extension.TrimStart('.') + ".spv")

    Write-Host "Compiling $($_.Name) -> $($outputFile)..."

    & $glslcPath $inputFile -o $outputFile

    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to compile $($_.Name)" -ForegroundColor Red
    } else {
        Write-Host "Successfully compiled $($_.Name)" -ForegroundColor Green
    }
}

Write-Host "Done."