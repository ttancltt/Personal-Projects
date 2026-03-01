
file(GLOB C_SOURCES "*.c")
target_sources(easypap PRIVATE ${C_SOURCES})
target_link_libraries(easypap PRIVATE m)