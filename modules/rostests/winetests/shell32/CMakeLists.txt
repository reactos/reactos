
add_definitions(
    -DWINETEST_USE_DBGSTR_LONGLONG
    -Dstrcasecmp=_stricmp
)

remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    appbar.c
    assoc.c
    autocomplete.c
    brsfolder.c
    ebrowser.c
    generated.c
    progman_dde.c
    recyclebin.c
    shelldispatch.c
    shelllink.c
    shellole.c
    shellpath.c
    shfldr_special.c
    shlexec.c
    shlfileop.c
    shlfolder.c
    shlview.c
    string.c
    systray.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(shell32_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    rsrc.rc)

target_link_libraries(shell32_winetest uuid)
set_module_type(shell32_winetest win32cui)
add_importlibs(shell32_winetest shell32 shlwapi ole32 oleaut32 user32 gdi32 advapi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(shell32_winetest ntdll)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(shell32_winetest PRIVATE -Wno-format-overflow)
endif()

add_pch(shell32_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET shell32_winetest)
