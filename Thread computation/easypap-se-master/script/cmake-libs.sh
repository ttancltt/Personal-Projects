#!/usr/bin/env bash

usage()
{
    echo "${BASH_SOURCE[0]##*/}: Compile ezv and ezm libraries"
    echo
    echo "Options:"
    echo "  -h | --help       : display help"
    echo "  -n | --no-install : compile without installing"
}

if (( $# > 0 )); then
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        --no-install|-n)
            DO_NOT_INSTALL=1
            ;;
        *)
            echo -e "Option $1 not recognized.\nFor more information, run:\n\t${BASH_SOURCE[0]} --help" 1>&2
            exit 1
            ;;
    esac
fi

# Usage examples:
#   INSTALL_DIR=/custom/path ./script/cmake-install-ezv-ezm.sh

BUILDDIR="${BUILDDIR:-build-libs}"
INSTALL_DIR="${INSTALL_DIR:-/tmp/easypap}"

rm -rf "$BUILDDIR"

# export CMAKE_BUILD_TYPE=Debug

# Configure (CONFIGURE_EASYPAP=OFF avoid using unnecessary dependencies on OpenSSL3, MPI, etc.
CC=${CC:-gcc} cmake -S . -B "$BUILDDIR" -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
                                  -DCONFIGURE_EASYPAP=OFF || exit $?

# Build ezv, ezm and easyview
cmake --build "$BUILDDIR" --parallel || exit $?

# install
if [[ -z ${DO_NOT_INSTALL+z} ]]; then
    cmake --install "$BUILDDIR" --component ezv || exit $?
    cmake --install "$BUILDDIR" --component ezm || exit $?
fi
