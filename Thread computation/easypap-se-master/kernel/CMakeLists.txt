add_subdirectory(c)

if(ENABLE_MIPP)
    add_subdirectory(mipp)
endif()

if(ENABLE_CUDA)
    add_subdirectory(cuda)
endif()
