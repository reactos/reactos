file(GLOB_RECURSE ARCH_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/*.c ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/*.S)
file(GLOB SOURCE "*.c")

add_library(pseh ${ARCH_SOURCE} ${SOURCE})
add_dependencies(pseh psdk)

