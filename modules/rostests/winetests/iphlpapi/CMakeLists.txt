
remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502)
add_definitions(-DWINVER=0x600 -D_WIN32_WINNT=0x600 -DNTDDI_VERSION=0x06000000)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)

add_executable(iphlpapi_winetest iphlpapi.c testlist.c)
target_link_libraries(iphlpapi_winetest wine)
set_module_type(iphlpapi_winetest win32cui)
add_importlibs(iphlpapi_winetest iphlpapi ws2_32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET iphlpapi_winetest)

if(MSVC)
    target_compile_options(iphlpapi_winetest PRIVATE
        /wd4024
        /wd4047
        /wd4133)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
   CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(iphlpapi_winetest PRIVATE
        -Wno-format
        -Wno-int-conversion
        -Wno-incompatible-pointer-types)
endif()

target_compile_definitions(iphlpapi_winetest PRIVATE 
    WINETEST_USE_DBGSTR_LONGLONG
    __WINE_WINSOCKAPI_STDLIB_H
    strcasecmp=_stricmp)

