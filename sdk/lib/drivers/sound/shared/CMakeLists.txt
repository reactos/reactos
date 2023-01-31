
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/sound/soundblaster
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/sound)

add_library(audio time.c)
add_dependencies(audio bugcodes xdk)
