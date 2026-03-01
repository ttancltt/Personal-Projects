
find_dependency(SDL2 REQUIRED)
find_dependency(OpenGL REQUIRED)
if (OFF)
  find_dependency(SCOTCH REQUIRED)
endif()
