#!/usr/bin/env bash

# Usage examples:
#   INSTALL_DIR=/custom/path ./script/cmake-from-external-easytools.sh

BUILDDIR="${BUILDDIR:-build-ext}"
INSTALL_DIR="${INSTALL_DIR:-/tmp/easypap}"

# Clean build directory
rm -rf "$BUILDDIR"
# Configure
CC=${CC:-gcc} CXX=${CXX:-g++} cmake -S . -B "$BUILDDIR" -DEXTERNAL_EASYTOOLS=ON -DEasypap_ROOT="${INSTALL_DIR}/lib/cmake/Easypap" || exit $?

# Build all
cmake --build "$BUILDDIR" --parallel || exit $?
