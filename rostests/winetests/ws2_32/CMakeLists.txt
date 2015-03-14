
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_executable(ws2_32_winetest protocol.c sock.c testlist.c)
set_module_type(ws2_32_winetest win32cui)
add_importlibs(ws2_32_winetest ws2_32 user32 msvcrt kernel32)
add_cd_file(TARGET ws2_32_winetest DESTINATION reactos/bin FOR all)

if(NOT MSVC)
    add_target_compile_flags(ws2_32_winetest "-Wno-format")
endif()
