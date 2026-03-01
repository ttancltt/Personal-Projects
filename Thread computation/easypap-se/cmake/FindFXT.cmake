#FindFXT.cmake 
# Ce module inclut les cibles suivantes 
#
# FXT::FXT

include(FindPackageHandleStandardArgs)

find_library(FXT_LIBRARY
    NAMES fxt 
    PATHS
        /usr/local/lib
        /usr/lib
        ${CMAKE_INSTALL_PREFIX}/lib
    HINTS
        ${FXT_ROOT_DIR}
)

# rechercher les entetes
find_path(FXT_INCLUDE_DIR
    NAMES
        fxt.h
    PATHS
        /usr/local/include
        /usr/local/include/fxt
        /usr/include
        /usr/include/fxt
        ${CMAKE_INSTALL_PREFIX}/include
    HINTS
        ${FXT_ROOT_DIR}/include
)

# verifier que fut.h y est aussi
if (NOT EXISTS "${FXT_INCLUDE_DIR}/fut.h")
    message(ERROR "fut.h not found")
endif()

# Gérer les arguments standard de FindPackage
find_package_handle_standard_args(FXT
    REQUIRED_VARS FXT_LIBRARY FXT_INCLUDE_DIR
)

# definir les cibles d'import

if (FXT_FOUND)
  add_library(FXT::FXT UNKNOWN IMPORTED)
  set_target_properties(FXT::FXT PROPERTIES
    IMPORTED_LOCATION ${FXT_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${FXT_INCLUDE_DIR}
  )
  mark_as_advanced(FXT_FOUND)
endif()
