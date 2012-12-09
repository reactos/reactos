
remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502)

add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    appbar.c
    assoc.c
    autocomplete.c
    brsfolder.c
    ebrowser.c
    generated.c
    #progman_dde.c FIXME: CORE-6559
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
    systray.c
    testlist.c
    rsrc.rc)

add_executable(shell32_winetest ${SOURCE})
target_link_libraries(shell32_winetest wine uuid)
set_module_type(shell32_winetest win32cui)
add_importlibs(shell32_winetest shell32 ole32 oleaut32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET shell32_winetest DESTINATION reactos/bin FOR all)
