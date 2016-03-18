
add_definitions(
    -D__NTDRIVER__
    -DKERNEL)

include_directories(
    ${REACTOS_SOURCE_DIR}/lib/drivers/sound/soundblaster
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/sound)

add_library(audioleg devname.c hardware.c)
add_dependencies(audioleg bugcodes xdk)
