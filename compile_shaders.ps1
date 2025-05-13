# C:/VulkanSDK/1.4.309.0/Bin/glslc.exe shaders/shader.vert -o spv/vert.spv
# C:/VulkanSDK/1.4.309.0/Bin/glslc.exe shaders/shader.frag -o spv/frag.spv

# This script compiles GLSL shaders to SPIR-V using glslc.exe
$shaderFolder = "shaders"

# Set the path to glslc.exe if it's not in PATH
$glslcPath = "C:\VulkanSDK\1.3.290.0\Bin\glslc.exe"  # or "glslc"

# Create output folder if it doesn't exist
$outputFolder = "spv"
if (-not (Test-Path $outputFolder)) {
    New-Item -ItemType Directory -Path $outputFolder
}

# Resolve absolute paths
$shaderFolderFull = (Resolve-Path $shaderFolder).Path
$outputFolderFull = (Resolve-Path $outputFolder).Path

# Recursively find shader files
Get-ChildItem -Path $shaderFolderFull -Recurse -File | ForEach-Object {
    $inputFile = $_.FullName

    # Get relative path from shader root folder
    $relativePath = $inputFile.Substring($shaderFolderFull.Length).TrimStart('\')

    # Get the directory and make sure it exists under output
    $relativeDir = Split-Path $relativePath
    $targetDir = Join-Path $outputFolderFull $relativeDir
    if (-not (Test-Path $targetDir)) {
        New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
    }

    # Build output filename (e.g., myshader_vert.spv)
    $outputFileName = ($_.BaseName + "_" + $_.Extension.TrimStart('.') + ".spv")
    $outputFile = Join-Path $targetDir $outputFileName

    Write-Host "Compiling $relativePath -> $($outputFile.Substring($outputFolderFull.Length + 1))..."

    & $glslcPath $inputFile -o $outputFile

    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to compile $relativePath" -ForegroundColor Red
    } else {
        Write-Host "Successfully compiled $relativePath" -ForegroundColor Green
    }
}

Write-Host "Done."