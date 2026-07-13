#!/bin/bash

set -e
mkdir build && cd build

echo "Running CMake..."
cmake ..
make -j$(nproc)

mkdir www
cp '../nginy.conf' nginy.conf

echo "Build successful!"