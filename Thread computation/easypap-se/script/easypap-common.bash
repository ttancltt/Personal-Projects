#!/usr/bin/env bash

_easypap_get_build_dir_candidates() # $1: name of array to fill $2: selected build dir
{
    local -n _dir_list=$1
    local -n _dir=$2
    local _d
    local _bdir

    # if BUILDDIR not defined, check if BUILD 
    if [[ -z ${BUILDDIR:+z} ]]; then
        case $BUILD in
            gcc*|clang*)
                BUILDDIR=build-${BUILD}
                ;;
        esac
    fi
    
    # Check BUILDDIR env variable in first place
    if [[ -n $BUILDDIR ]]; then
        BUILDDIR="${BUILDDIR%/}"  # Remove trailing slash if any
        if [[ "${BUILDDIR##*/}" == "$BUILDDIR" ]]; then
            _bdir="${EASYPAPDIR}/${BUILDDIR}"
        else
            _bdir="${BUILDDIR}"
        fi
        if [[ ! -d "${_bdir}" ]]; then
            [[ -n $EASYPAP_VERBOSE ]] && echo "Error: Specified BUILDDIR directory ${_bdir} does not exist." >&2
            return 1
        fi
        # Add $BUILDDIR if not a build* directory
        if [[ ! $BUILDDIR =~ ^build* ]]; then
            _dir_list+=("${_bdir}")
        fi
    fi

    # Then check conf file from build-select script
    if [[ -z ${_bdir+z} && -f "${EASYPAPDIR}/.easypap-build-dir" ]]; then
        _bdir=$(< "${EASYPAPDIR}/.easypap-build-dir")
        _bdir="${_bdir%/}"  # Remove trailing slash if any
        if [[ "${_bdir##*/}" == "$_bdir" ]]; then
            _bdir="${EASYPAPDIR}/${_bdir}"
        fi
        if [[ ! -d "${_bdir}" ]]; then
            [[ -n $EASYPAP_VERBOSE ]] && echo "Error: Build directory set by build-select (${_bdir}) does not exist." >&2
            [[ -n $EASYPAP_VERBOSE ]] && echo "       Use build-select --unset to clear the invalid build directory." >&2
            return 1
        fi
        # Add dir if not a build* directory
        if [[ ! "${_bdir}" =~ ^${EASYPAPDIR}/build* ]]; then
            _dir_list+=("${_bdir}")
        fi
    fi

    for _d in "${EASYPAPDIR}"/build*; do
        if [[ -d $_d ]]; then
            _dir_list+=("$_d")
            if [[ -z ${_bdir+z} ]]; then
                _bdir=$_d
            fi
        fi
    done

    _dir="$_bdir"

    [[ ${#_dir_list[@]} -gt 0 ]] && return 0

    [[ -n $EASYPAP_VERBOSE ]] && echo "Error: No build directory found." >&2
    return 1
}

_easypap_find_executable() # $1: executable name $2: name of variable to fill with path
{
    local -n _executable=$2
    local _build_dir

    _easypap_get_build_dir_candidates _ _build_dir || return 1

    if [[ -x "${_build_dir}/bin/$1" ]]; then
        _executable="${_build_dir}/bin/$1"
        return 0
    fi

    [[ -n $EASYPAP_VERBOSE ]] && echo "Error: Could not find $1 in ${_build_dir}/bin" >&2
    return 1
}

_easypap_find()
{
    _easypap_find_executable easypap EASYPAP
}

_easyview_find()
{
    _easypap_find_executable easyview EASYVIEW
}

EASYPAPDIR=${EASYPAPDIR:-.}

TRACEDIR=${TRACEDIR:-${EASYPAPDIR}/data/traces}
CUR_TRACEFILE=ezv_trace_current.evt
PREV_TRACEFILE=ezv_trace_previous.evt

HASHDIR=${EASYPAPDIR}/data/hash
DUMPDIR=${EASYPAPDIR}/data/dump
PERFDIR=${EASYPAPDIR}/data/perf
