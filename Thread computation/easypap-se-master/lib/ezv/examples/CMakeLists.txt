
set(EZV_TESTS ezv_img.c ezv_mesh.c)

foreach(src ${EZV_TESTS})
    get_filename_component(exec ${src} NAME_WE)
    add_executable(${exec} ${src})
    set_property(TARGET ${exec} PROPERTY
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
    )
    target_link_libraries(${exec} ezv)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" AND CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${exec} PRIVATE -march=native)
    endif()
endforeach()
