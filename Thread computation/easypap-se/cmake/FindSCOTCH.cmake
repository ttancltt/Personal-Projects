# - Try to find SCOTCH
# Once done, this will define
#
#  SCOTCH_FOUND - system has SCOTCH
#  SCOTCH_INCLUDE_DIRS - the SCOTCH include directories
#  SCOTCH_LIBRARIES - link these to use SCOTCH
#  SCOTCH_DEFINITIONS - compile definitions for using SCOTCH

find_path(SCOTCH_INCLUDE_DIR
  NAMES
    scotch.h
  PATHS
    /usr/local/include
    /usr/local/include/scotch
    /usr/include
    /usr/include/scotch
    ${CMAKE_INSTALL_PREFIX}/include
)

find_library(SCOTCH_LIBRARY
  NAMES scotch
  PATHS
    /usr/local/lib
    /usr/lib
    ${CMAKE_INSTALL_PREFIX}/lib
)

find_library(SCOTCHERR_LIBRARY
  NAMES scotcherr
  PATHS
    /usr/local/lib
    /usr/lib
    ${CMAKE_INSTALL_PREFIX}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SCOTCH DEFAULT_MSG
  SCOTCH_INCLUDE_DIR SCOTCH_LIBRARY SCOTCHERR_LIBRARY)

if(SCOTCH_FOUND)
  set(SCOTCH_LIBRARIES ${SCOTCH_LIBRARY} ${SCOTCHERR_LIBRARY})
  set(SCOTCH_INCLUDE_DIRS ${SCOTCH_INCLUDE_DIR})

  add_library(SCOTCH::scotch UNKNOWN IMPORTED)
  set_target_properties(SCOTCH::scotch PROPERTIES
    IMPORTED_LOCATION ${SCOTCH_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${SCOTCH_INCLUDE_DIRS}
  )

  add_library(SCOTCH::scotcherr UNKNOWN IMPORTED)
  set_target_properties(SCOTCH::scotcherr PROPERTIES
    IMPORTED_LOCATION ${SCOTCHERR_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${SCOTCH_INCLUDE_DIRS}
  )
endif()

mark_as_advanced(SCOTCH_INCLUDE_DIR SCOTCH_LIBRARY SCOTCHERR_LIBRARY)
