#!/usr/bin/env bash

INSTALL_DIR="${INSTALL_DIR:-/tmp/easypap}"

rm -rf build

# export CMAKE_BUILD_TYPE=Debug

CC=${CC:-gcc} cmake -S . -B build -DEasypap_ROOT=${INSTALL_DIR}/lib/cmake/Easypap || exit $?

cmake --build build --parallel || exit $?
