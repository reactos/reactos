
add_definitions(-D__NTDRIVER__)
add_definitions(-DKERNEL)

include_directories(${REACTOS_SOURCE_DIR}/lib/drivers/sound/soundblaster)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/sound)

add_library(audio time.c)
add_dependencies(audio bugcodes)
