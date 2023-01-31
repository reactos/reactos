
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/sound/soundblaster
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/sound)

add_library(audioleg devname.c hardware.c)
add_dependencies(audioleg bugcodes xdk)
