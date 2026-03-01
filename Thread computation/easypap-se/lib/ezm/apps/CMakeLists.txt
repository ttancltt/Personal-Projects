cmake_minimum_required(VERSION 3.18)
project(mesh LANGUAGES C)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Ensure find_package prefers xxxConfig.cmake over Findxxx.cmake
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

# Set CMake policies
cmake_policy(SET CMP0072 NEW) # Prefer legacy OpenGL over GLVND
cmake_policy(SET CMP0074 NEW) # Use <PackageName>_ROOT variables for find_package

# We need ezm (ezv will be included automatically)
find_package(Easypap COMPONENTS ezm REQUIRED)
find_package(OpenMP REQUIRED)

set(PSOURCES
    mesh.c
    mandel.c
)

foreach(src ${PSOURCES})
    get_filename_component(exec ${src} NAME_WE)
    add_executable(${exec} ${src})
    target_link_libraries(${exec} PRIVATE Easypap::ezm OpenMP::OpenMP_C)
    target_compile_options(${exec} PRIVATE -Wall -Wno-unused-function)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" AND CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${exec} PRIVATE -march=native)
    endif()
endforeach()
