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
        BUILDDIR=build-${BUILD}
        ;;
    clang*)
        CC=clang
        CXX=clang++
        BUILDDIR=build-${BUILD}
        ;;
esac

BUILDDIR="${BUILDDIR:-build}"

# Clean build directory
rm -rf "$BUILDDIR"

# Configure
CC=${CC:-gcc} CXX=${CXX:-g++} cmake -S . -B "$BUILDDIR" || exit $?

# Build all
cmake --build "$BUILDDIR" --parallel || exit $?
