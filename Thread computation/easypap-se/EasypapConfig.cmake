
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was EasypapConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

set(_Easypap_supported_components ezv ezm)

# If no component has been requested, include them all
if (NOT Easypap_FIND_COMPONENTS)
  set(Easypap_FIND_COMPONENTS ${_Easypap_supported_components})
endif()

# ezm depends on ezv
if ("ezm" IN_LIST Easypap_FIND_COMPONENTS)
  if (NOT "ezv" IN_LIST Easypap_FIND_COMPONENTS)
    list (PREPEND Easypap_FIND_COMPONENTS "ezv")
    message (STATUS "Adding ezv to dependency list")
  endif()
endif()

include(CMakeFindDependencyMacro)

# Add the current directory to CMAKE_MODULE_PATH
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

foreach(_comp ${Easypap_FIND_COMPONENTS})
  if (NOT _comp IN_LIST _Easypap_supported_components)
    set(Easypap_FOUND False)
    set(Easypap_NOT_FOUND_MESSAGE "Unsupported component: ${_comp}")
  endif()
  include("${CMAKE_CURRENT_LIST_DIR}/${_comp}Targets.cmake")
  include("${CMAKE_CURRENT_LIST_DIR}/${_comp}Config.cmake")
endforeach()
