cmake_minimum_required(VERSION 3.18)
project(examples LANGUAGES C)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Ensure find_package prefers xxxConfig.cmake over Findxxx.cmake
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

# Set CMake policies
cmake_policy(SET CMP0072 NEW) # Prefer legacy OpenGL over GLVND
cmake_policy(SET CMP0074 NEW) # Use <PackageName>_ROOT variables for find_package

# We only need ezv from EasyPap
find_package(Easypap COMPONENTS ezv REQUIRED)

set(PSOURCES
    boolmat.c       cellmask.c     diy.c           imagecpu.c      mesh.c          morton.c        part.c          store.c
    cpu.c           dualimage.c    image.c         imagedapu.c     monitor.c       paint.c         patch.c         thr.c
)

foreach(src ${PSOURCES})
    get_filename_component(exec ${src} NAME_WE)
    add_executable(${exec} ${src})
    target_link_libraries(${exec} Easypap::ezv m)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" AND CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${exec} PRIVATE -march=native)
    endif()
endforeach()
