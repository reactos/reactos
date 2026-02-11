
remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502)
add_definitions(-DWINVER=0x600 -D_WIN32_WINNT=0x600 -DNTDDI_VERSION=0x06000000)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)

list(APPEND SOURCE
    afd.c
    protocol.c
    sock.c
    testlist.c)

add_executable(ws2_32_winetest 
    ${SOURCE})

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(ws2_32_winetest PRIVATE -Wno-format -Wno-unused)
elseif(MSVC)
    target_compile_options(ws2_32_winetest PRIVATE /wd4101)
endif()

set_module_type(ws2_32_winetest win32cui)
add_importlibs(ws2_32_winetest ws2_32 user32 msvcrt kernel32 iphlpapi ntdll)
add_rostests_file(TARGET ws2_32_winetest)
