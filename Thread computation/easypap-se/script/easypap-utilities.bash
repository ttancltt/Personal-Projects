#!/usr/bin/env bash

. "${BASH_SOURCE%/*}/easypap-common.bash"

# result placed in kernels
_easypap_kernels()
{
    local symbol

    kernels=

    _easypap_find || return 1

    while read _ _ symbol; do
        case $symbol in
            *_compute_seq)
                symbol=${symbol#_}
                symbol=${symbol%_compute_seq}
                kernels="$kernels $symbol" ;;
            *) continue ;;
        esac
    done < <(nm "${EASYPAP}")

    [[ -n $kernels ]] # return 0 if not empty, 1 otherwise
}

_easypap_grep_variants() # kernel func result
{
    local symbol
    local kernel=$1
    local func=$2
    local -n __result=$3

    while read _ _ symbol; do
        if [[ $symbol =~ ^[_]?${kernel}_${func}_ ]]; then
                symbol=${symbol#*_${func}_}
                case $symbol in
                    ocl*|cuda*|*.*) continue ;;
                    *) __result="$__result $symbol" ;;
                esac
        fi
    done < <(nm "${EASYPAP}")
}

# result placed in variants
_easypap_variants()
{
    variants=

    if (( $# == 0 )); then
        set none
    fi

    _easypap_find || return 1

    _easypap_grep_variants $1 compute variants

    [[ -n $variants ]] # return 0 if not empty, 1 otherwise
}

# result places in gpu_flavor
_easypap_gpu_flavor()
{
    gpu_flavor=$(./run --gpu-flavor)

    [[ -n $gpu_flavor ]] # return 0 if not empty, 1 otherwise
}

# result placed in gvariants
_easypap_opencl_variants()
{
    local f tmp

    gvariants=

    # The most secure way of finding GPU kernels is to ask easypap…
    tmp=$(./run -k $1 -lgv 2> /dev/null)

    # But the "grep into the .cl file" method is much much faster!
    #tmp=`awk '/__kernel/ {print $3}' < kernel/ocl/${k}.cl`

    for f in $tmp; do
        if [[ $f =~ ^$1_ocl* ]]; then
            gvariants="$gvariants ${f#$1_}"
        fi
    done
    
    [[ -n $gvariants ]] # return 0 if not empty, 1 otherwise
}

# result placed in gvariants
_easypap_cuda_variants()
{
    local f tmp
    local kernel=$1

    gvariants=

    _easypap_find || return 1

    while read _ _ symbol; do
        if [[ $symbol =~ ^[_]?${kernel}_cuda ]]; then
                symbol=${symbol#${kernel}_}
                case $symbol in
                    *.*) continue ;;
                    *) gvariants="$gvariants $symbol" ;;
                esac
        fi
    done < <(nm "${EASYPAP}")

    [[ -n $gvariants ]] # return 0 if not empty, 1 otherwise
}

# result placed in gvariants
_easypap_gpu_variants()
{
    if (( $# == 0 )); then
        set none
    fi

    _easypap_gpu_flavor || return 1

    case $gpu_flavor in
        ocl)
            _easypap_opencl_variants $1
            ;;
        cuda)
            _easypap_cuda_variants $1
            ;;
        *)
            ;;
    esac    
}

# result placed in draw_funcs
_easypap_draw_funcs()
{
    draw_funcs=

    if (( $# == 0 )); then
        set none
    fi

    _easypap_find || return 1

    _easypap_grep_variants $1 draw draw_funcs

    [[ -n $draw_funcs ]] # return 0 if not empty, 1 otherwise
}

# result placed in tile_funcs
_easypap_tile_funcs()
{
    tile_funcs=

    if (( $# == 0 )); then
        set none
    fi

    _easypap_find || return 1

    local kernel=$1
    local symbol

    while read _ _ symbol; do
        if [[ $symbol =~ ^[_]?${kernel}_(do_tile|do_patch)_ ]]; then
                symbol=${symbol#*_do_tile_}
                symbol=${symbol#*_do_patch_}
                case $symbol in
                    *.*) continue ;;
                    *) tile_funcs="$tile_funcs $symbol" ;;
                esac
        fi
    done < <(nm "${EASYPAP}")

    [[ -n $tile_funcs ]] # return 0 if not empty, 1 otherwise
}

# result placed in pos
_easypap_option_position()
{
    local p

    if [[ $1 =~ ^--.* ]]; then
	    list=("${LONG_OPTIONS[@]}")
    else
	    list=("${SHORT_OPTIONS[@]}")
    fi
    for (( p=0; p < $NB_OPTIONS; p++ )); do
	    if [[ "${list[p]}" = "$1" ]]; then
	        pos=$p
	        return 0
	    fi
    done
    pos=$NB_OPTIONS

    return 1
}

_easypap_remove_from_suggested()
{
    local i
    for (( i=0; i < ${#suggested[@]}; i++ )); do 
        if [[ ${suggested[i]} == $1 ]]; then
            suggested=( "${suggested[@]:0:$i}" "${suggested[@]:$((i + 1))}" )
            i=$((i - 1))
        fi
    done

    return 0
}

_easypap_option_suggest()
{
    local c e o suggested=("$@")

    # We should prune the options than should only appear at first position    
    for (( o=0; o < $NB_OPTIONS; o++ )); do
        local short=${SHORT_OPTIONS[o]}
        local only_in_first_place_opt=only_in_first_place_${short#-}
        only_in_first_place_opt=${!only_in_first_place_opt}
        if (( only_in_first_place_opt == 1 )); then
            _easypap_remove_from_suggested ${LONG_OPTIONS[o]}
            _easypap_remove_from_suggested $short
           fi
    done

    for (( c=1; c < $COMP_CWORD; c++ )); do
	    if [[ ${COMP_WORDS[c]} =~ ^-.* ]]; then
	        _easypap_option_position ${COMP_WORDS[c]}
	        if (( pos < NB_OPTIONS )); then
	            local short=${SHORT_OPTIONS[pos]}
                local multiple_opt=multiple_${short#-}
                multiple_opt=${!multiple_opt}

                if [[ -z $multiple_opt ]]; then
		            # we shall remove this option from suggested options
                    _easypap_remove_from_suggested ${LONG_OPTIONS[pos]}
                    _easypap_remove_from_suggested $short
                fi

		        # also remove antagonist options
                local excluding_opt="exclude_${short#-}[@]"
                excluding_opt=("${!excluding_opt}")

		        for ((e=0; e < ${#excluding_opt[@]}; e++ )); do
		            local p=${excluding_opt[e]}
                    _easypap_remove_from_suggested ${LONG_OPTIONS[p]}
                    _easypap_remove_from_suggested ${SHORT_OPTIONS[p]}
		        done
	        fi
	    fi
    done

    COMPREPLY=($(compgen -W '"${suggested[@]}"' -- $cur))

    return 0
}
