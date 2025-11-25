#!/bin/bash

# compile the program
# make

# Create output directory if it doesn't exist
mkdir -p testfiles/output

# Define colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Function to run a test case
run_test() {
    local test_id=$1
    local cmd=$2

    echo "Running Test $test_id..."
    
    # run the command and redirect both stdout and stderr to the output file
    $cmd > "testfiles/output/$test_id" 2>&1
    
    # compare output with expected ones /testfiles/expected
    if diff "testfiles/expected/$test_id" "testfiles/output/$test_id" > /dev/null; then
        echo -e "${GREEN}Test $test_id passed${NC}"
    else
        echo -e "${RED}Test $test_id failed${NC}"
        echo "Diff (Expected vs Actual):"
        diff -u "testfiles/expected/$test_id" "testfiles/output/$test_id"
        exit 1
    fi
}

# --- Test cases invalid prompt ---

# Test 1.1: No arguments
run_test "1.1" "./fatrec32"

# Test 1.2: Only -i flag (insufficient arguments)
run_test "1.2" "./fatrec32 -i"

# Test 1.3: Only -l flag (insufficient arguments)
run_test "1.3" "./fatrec32 -l"

# Test 1.4: Only -l flag (insufficient arguments)
run_test "1.4" "./fatrec32 -r"

# Test 1.5: Only -l flag (insufficient arguments)
run_test "1.5" "./fatrec32 -R"

# Test 1.6: Only -l flag (insufficient arguments)
run_test "1.6" "./fatrec32 -ra"

# Test 1.7: Only -l flag (insufficient arguments)
run_test "1.7" "./fatrec32 -all"

# TODO: ADD MORE TEST CASES FOR INVALID INPUT FORMAT