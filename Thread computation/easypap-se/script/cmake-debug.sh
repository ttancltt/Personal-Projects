#!/usr/bin/env bash

# Usage examples:
#   ./script/cmake-easypap.sh
#   BUILD=gcc ./script/cmake-easypap.sh
#   BUILD=clang ./script/cmake-easypap.sh

# shortcut
case $BUILD in
    gcc*)
        CC=gcc
        CXX=g++
        ;;
    clang*)
        CC=clang
        CXX=clang++
        ;;
esac

BUILDDIR="${BUILDDIR:-build-debug}"

# Clean build directory
rm -rf "$BUILDDIR"

# Configure
CC=${CC:-gcc} CXX=${CXX:-g++} cmake -S . -B "$BUILDDIR" -DCMAKE_BUILD_TYPE=Debug || exit $?

# Build all
cmake --build "$BUILDDIR" --parallel || exit $?
