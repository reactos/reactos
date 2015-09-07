
add_definitions(
    -D__NTDRIVER__
    -DKERNEL)

include_directories(
    ${REACTOS_SOURCE_DIR}/lib/drivers/sound/soundblaster
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/sound)

add_library(uartmidi midiuart.c)
add_dependencies(uartmidi bugcodes xdk)
