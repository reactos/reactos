
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

add_executable(ws2_32_winetest protocol.c sock.c testlist.c)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(ws2_32_winetest PRIVATE "-Wno-format")
endif()

set_module_type(ws2_32_winetest win32cui)
add_importlibs(ws2_32_winetest ws2_32 user32 msvcrt kernel32)
add_rostests_file(TARGET ws2_32_winetest)
