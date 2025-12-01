#!/bin/bash

# compile the program
# make

# Clean up previous output and create directory
rm -rf testfiles/output
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
    eval "$cmd" > "testfiles/output/$test_id" 2>&1
    
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

# Checksum
verify_checksum() {
    local file=$1
    local expected=$2
    local actual=$(shasum "$file" | awk '{print $1}')
    
    if [ "$expected" != "$actual" ]; then
        echo -e "${RED}Checksum verification failed for $file${NC}"
        echo "Expected: $expected"
        echo "Actual:   $actual"
        exit 1
    else
        echo -e "${GREEN}Checksum verification passed for $file${NC}"
    fi
}

verify_checksum "disks/sample.disk" "aee1ff64579ea30094ddf08a284dfed7e3e35af0"
verify_checksum "disks/single_recovery.disk" "530d7f80fa7d825ec92efc6299db43e639606c14"
verify_checksum "disks/multi_recovery.disk" "27db18dfc76d015b519e4d63483fb79b3d652476"
verify_checksum "disks/all_recovery.disk" "e9e664b24233a980326c03008e7c781df57a2e21"

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

# Test 2.1: System Information
run_test "2.1" "./fatrec32 disks/sample.disk -i"
# Test 2.2: List entries
run_test "2.2" "./fatrec32 disks/sample.disk -l"

# Test 2.3: Recover FILE1.TXT from fat32 single_recovery.dis
# Create a temp copy of the disk to run tests in docker
cp disks/single_recovery.disk disks/test_run.disk

# Test 2.3: List files on the disk BEFORE recovery (should show deleted state)
run_test "2.3" "./fatrec32 disks/test_run.disk -l"

# Test 2.4: Recover FILE1.TXT
run_test "2.4" "./fatrec32 disks/test_run.disk -r FILE1.TXT"

# Test 2.5: List files on the recovered disk to verify FILE1.TXT is present
run_test "2.5" "./fatrec32 disks/test_run.disk -l"

# Test 2.6: Recover FILE1.TXT with SHA1 validation
# we need to reset the disk first since 2.4 already recovered it
cp disks/single_recovery.disk disks/test_run_sha1.disk
run_test "2.6" "./fatrec32 disks/test_run_sha1.disk -r FILE1.TXT -s cc550c2c72999d7db967cd71cb7c1e8f60e332c1"
rm disks/test_run_sha1.disk

# Clean up temp test disk
rm disks/test_run.disk

# Test 3.1: Recover FILE2.TXT from a disk with multi files
cp disks/multi_recovery.disk disks/test_run_multi.disk

# Test 3.1: List files on the disk BEFORE recovery (should show FILE1.TXT present, FILE2.TXT deleted)
run_test "3.1" "./fatrec32 disks/test_run_multi.disk -l"

# Test 3.2: Recover FILE2.TXT
run_test "3.2" "./fatrec32 disks/test_run_multi.disk -r FILE2.TXT"

# Test 3.3: List files on the recovered disk to verify both files are present (should show FILE1.txt and FILE2.TXT)
run_test "3.3" "./fatrec32 disks/test_run_multi.disk -l"

# Clean up
rm disks/test_run_multi.disk

# Test 4.1: Recover all deleted files from the fat32 disk (kept all files from /testfiles/files/* except IMAGE.png and NOTHING.txt)
cp disks/all_recovery.disk disks/test_run_all.disk

# Test 4.1: List all the files on the disk BEFORE recovery (should not show IMAGE.png and NOTHING.txt)
run_test "4.1" "./fatrec32 disks/test_run_all.disk -l"

# Test 4.2: Recover all deleted files
run_test "4.2" "./fatrec32 disks/test_run_all.disk -all"

# Test 4.3: List files on the recovered disk to verify all the files (of testfiles/files/*) are now recovered
run_test "4.3" "./fatrec32 disks/test_run_all.disk -l"

# Clean up
rm disks/test_run_all.disk
