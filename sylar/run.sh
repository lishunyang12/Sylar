#!/bin/bash

# Clear previous build files to avoid cache conflicts
# Always do this when you modify CMakeLists.txt or in a new complication env to ensure fresh compilation
rm -rf build

# Create and enter build directory
# -p flag creates parent directories if needed
mkdir -p build && cd build

# Generate build system using CMake
# -DCMAKE_BUILD_TYPE=Release enables optimizations
# The ".." means CMakeLists.txt is in parent directory
cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)

# Run test program
# First go to bin directory where executables are output
# Then execute the test_config binary
cd ../bin && ./test_iomanager

# Return to project root directory
# This maintains consistent working directory for subsequent operations
cd ..