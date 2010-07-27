
add_definitions(-D_NTSYSTEM_)

file(GLOB_RECURSE SOURCE "*.c")

add_library(rossym ${SOURCE})