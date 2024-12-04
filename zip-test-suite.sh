TEST_SUITE="$1"
SUITE_DIR="test-suites/${TEST_SUITE}-test-suite"
ZIP_NAME="${TEST_SUITE}-test-suite.zip"

# Check if directory exists
if [ ! -d "$SUITE_DIR" ]; then
    echo "Error: Test suite directory not found: $SUITE_DIR"
    exit 1
fi

# Save current directory
CURRENT_DIR=$(pwd)

# Change to suite directory
cd "$SUITE_DIR" || exit 1

# Remove existing zip if it exists
if [ -f "../$ZIP_NAME" ]; then
    rm "../$ZIP_NAME" || {
        echo "Error: Failed to remove existing zip file"
        cd "$CURRENT_DIR"
        exit 1
    }
fi

# Create zip without parent directory
zip -r "../$ZIP_NAME" ./* || {
    echo "Error: Failed to create zip file"
    cd "$CURRENT_DIR"
    exit 1
}

# Return to original directory
cd "$CURRENT_DIR"

echo "Created $ZIP_NAME successfully"