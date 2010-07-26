include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/fullfat)

add_definitions(-D__NTDRIVER__)
file(GLOB_RECURSE SOURCE "*.c")

add_library(fullfat ${SOURCE})