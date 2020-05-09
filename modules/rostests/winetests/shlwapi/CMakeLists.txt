
add_definitions(
    -DUSE_WINE_TODOS
    -DWINETEST_USE_DBGSTR_LONGLONG)

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
endif()

add_pch(shlwapi_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET shlwapi_winetest)
