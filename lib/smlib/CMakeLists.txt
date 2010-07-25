include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

file(GLOB_RECURSE SOURCE "*.c")

add_library(smlib ${SOURCE})