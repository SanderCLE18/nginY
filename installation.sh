#!/bin/bash

set -e
mkdir build && cd build

echo "Running CMake..."
cmake ..
make -j$(nproc)

mkdir www
mv '../nginy.config' nginy.config

echo "Build successful!"