#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Easypap::ezv" for configuration "Release"
set_property(TARGET Easypap::ezv APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Easypap::ezv PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libezv.so"
  IMPORTED_SONAME_RELEASE "libezv.so"
  )

list(APPEND _cmake_import_check_targets Easypap::ezv )
list(APPEND _cmake_import_check_files_for_Easypap::ezv "${_IMPORT_PREFIX}/lib/libezv.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
