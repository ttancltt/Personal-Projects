#!/usr/bin/env bash

# Usage examples:
#   INSTALL_DIR=/custom/path ./script/cmake-install-ezv-ezm.sh

BUILDDIR="${BUILDDIR:-build-install}"
INSTALL_DIR="${INSTALL_DIR:-/tmp/easypap}"

rm -rf "$BUILDDIR"

# export CMAKE_BUILD_TYPE=Debug

# Configure (CONFIGURE_EASYPAP=OFF avoid using unnecessary dependencies on OpenSSL3, MPI, etc.
CC=${CC:-gcc} cmake -S . -B "$BUILDDIR" -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
                                  -DCONFIGURE_EASYPAP=OFF || exit $?

# Build ezv, ezm and easyview
cmake --build "$BUILDDIR" --parallel || exit $?
# install
#rm -rf ${INSTALL_DIR}
cmake --install "$BUILDDIR" --component ezv || exit $?
cmake --install "$BUILDDIR" --component ezm || exit $?
