#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Easypap::ezm" for configuration "Release"
set_property(TARGET Easypap::ezm APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Easypap::ezm PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libezm.so"
  IMPORTED_SONAME_RELEASE "libezm.so"
  )

list(APPEND _cmake_import_check_targets Easypap::ezm )
list(APPEND _cmake_import_check_files_for_Easypap::ezm "${_IMPORT_PREFIX}/lib/libezm.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
