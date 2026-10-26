
add_definitions(-DPC_NO_IMPORTS)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/sound
    ${REACTOS_SOURCE_DIR}/drivers/wdm/audio/legacy/wdmaud)

add_executable(audio_test audio_test.c)
set_module_type(audio_test win32cui)
add_importlibs(audio_test setupapi ksuser msvcrt kernel32)
add_dependencies(audio_test bugcodes)
