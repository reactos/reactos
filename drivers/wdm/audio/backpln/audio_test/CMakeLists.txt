
add_definitions(-DPC_NO_IMPORTS)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/sound)
include_directories(${REACTOS_SOURCE_DIR}/drivers/wdm/audio/legacy/wdmaud)
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/libsamplerate)

add_executable(audio_test audio_test.c)

set_module_type(audio_test win32cui)

target_link_libraries(audio_test
    libsamplerate
    mingw_main
    mingw_common)

add_importlibs(audio_test setupapi ksuser msvcrt)
add_dependencies(audio_test psdk bugcodes)
