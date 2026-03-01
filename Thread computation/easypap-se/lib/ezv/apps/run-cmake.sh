#!/usr/bin/env bash

rm -rf build

# export CMAKE_BUILD_TYPE=Debug

CC=${CC:-gcc} cmake -S . -B build -DEasypap_ROOT=/tmp/easypap/lib/cmake/Easypap || exit $?

cmake --build build --parallel || exit $?

