#!/bin/bash

# This script compiles GLSL shaders to SPIR-V using glslangValidator

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Path to shader source folder
SHADER_FOLDER="shaders"

# Path to glslangValidator (make sure it's in your PATH or set full path here)
GLSLANG_PATH="glslangValidator"

# Output folder
OUTPUT_FOLDER="spv"

# Create output folder if it doesn't exist
mkdir -p "$OUTPUT_FOLDER"

# Resolve full paths
SHADER_FOLDER_FULL=$(realpath "$SHADER_FOLDER")
OUTPUT_FOLDER_FULL=$(realpath "$OUTPUT_FOLDER")

# Find all shader files
find "$SHADER_FOLDER_FULL" -type f \( -name "*.vert" -o -name "*.frag" -o -name "*.comp" -o -name "*.geom" -o -name "*.tesc" -o -name "*.tese" \) | while read -r INPUT_FILE; do
    # Get relative path
    RELATIVE_PATH="${INPUT_FILE#$SHADER_FOLDER_FULL/}"
    RELATIVE_DIR=$(dirname "$RELATIVE_PATH")
    TARGET_DIR="$OUTPUT_FOLDER_FULL/$RELATIVE_DIR"
    mkdir -p "$TARGET_DIR"

    # Get base filename
    BASENAME=$(basename "$INPUT_FILE")
    EXT="${BASENAME##*.}"
    NAME="${BASENAME%.*}"

    OUTPUT_FILE="$TARGET_DIR/${NAME}.${EXT}.spv"

    echo -e "${BLUE}Compiling ${YELLOW}$RELATIVE_PATH${NC} -> ${GREEN}${OUTPUT_FILE#$OUTPUT_FOLDER_FULL/}${NC}..."

    "$GLSLANG_PATH" -V "$INPUT_FILE" -o "$OUTPUT_FILE"
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to compile ${RELATIVE_PATH}${NC}" >&2
    else
        echo -e "${GREEN}Successfully compiled ${RELATIVE_PATH}${NC}"
    fi
done

echo -e "${CYAN}Done.${NC}"