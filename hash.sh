#!/bin/bash

# Check if the file path is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <file_path>"
    exit 1
fi

file_path="$1"

# Resolve the full path of the file
full_path=$(realpath "$file_path" 2>/dev/null)

# Check if the file exists
if [ -f "$full_path" ]; then
    # Calculate and print the SHA-256 hash of the file
    sha256sum "$full_path" | awk '{ print $1 }'
else
    echo "File does not exist: $file_path"
    exit 1
fi