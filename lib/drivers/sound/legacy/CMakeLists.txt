
add_definitions(-D__NTDRIVER__)
add_definitions(-DKERNEL)

include_directories(${REACTOS_SOURCE_DIR}/lib/drivers/sound/soundblaster)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/sound)

file(GLOB_RECURSE SOURCE "*.c")

add_library(audioleg ${SOURCE})
add_dependencies(audioleg bugcodes)
