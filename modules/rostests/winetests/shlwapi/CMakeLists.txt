
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)
remove_definitions(-DWINVER=0x502 -D_WIN32_WINNT=0x502)
add_definitions(-DWINVER=0x0600 -D_WIN32_WINNT=0x0600 -DNTDDI_VERSION=0x06000100)

add_definitions(
    -DUSE_WINE_TODOS
    -DWINETEST_USE_DBGSTR_LONGLONG
    -Dstrdup=_strdup)

list(APPEND SOURCE
    assoc.c
    clist.c
    clsid.c
    generated.c
    istream.c
    ordinal.c
    path.c
    shreg.c
    string.c
    thread.c
    url.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(shlwapi_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

target_link_libraries(shlwapi_winetest uuid)
set_module_type(shlwapi_winetest win32cui)
add_importlibs(shlwapi_winetest shlwapi shell32 mlang oleaut32 ole32 user32 advapi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(shlwapi_winetest ntdll)
    target_compile_options(shlwapi_winetest PRIVATE
        /wd4113
        /wd4028
        /utf-8)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
   CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(shlwapi_winetest PRIVATE 
        -Wno-format
        -Wno-discarded-qualifiers
        -Wno-incompatible-pointer-types)
endif()

add_pch(shlwapi_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET shlwapi_winetest)
