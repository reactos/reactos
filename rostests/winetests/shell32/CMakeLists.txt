
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x501)

add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    appbar.c
    autocomplete.c
    generated.c
    progman_dde.c
    shelllink.c
    shellpath.c
    shlexec.c
    shlfileop.c
    shlfolder.c
    shlview.c
    string.c
    systray.c
    testlist.c
    shfldr_special.c
    rsrc.rc)

add_executable(shell32_winetest ${SOURCE})
target_link_libraries(shell32_winetest wine uuid)
set_module_type(shell32_winetest win32cui)
add_importlibs(shell32_winetest shlwapi gdi32 shell32 ole32 oleaut32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET shell32_winetest DESTINATION reactos/bin FOR all)
