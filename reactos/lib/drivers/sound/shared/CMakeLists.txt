
add_definitions(
    -D__NTDRIVER__
    -DKERNEL)

include_directories(
    ${REACTOS_SOURCE_DIR}/lib/drivers/sound/soundblaster
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/sound)

add_library(audio time.c)
add_dependencies(audio bugcodes xdk)
