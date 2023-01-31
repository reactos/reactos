
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/sound/soundblaster
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/sound)

add_library(uartmidi midiuart.c)
add_dependencies(uartmidi bugcodes xdk)
