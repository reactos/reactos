
set_unicode()

list(APPEND SOURCE
    aboutdlg.c
    available.c
    installdlg.c
    installed.c
    listview.c
    loaddlg.c
    misc.c
    parser.c
    richedit.c
    settingsdlg.c
    splitter.c
    statusbar.c
    toolbar.c
    treeview.c
    winmain.c
    rapps.rc)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${REACTOS_BINARY_DIR}/include/reactos)

set_rc_compiler()

add_executable(rapps ${SOURCE})
set_module_type(rapps win32gui)
target_link_libraries(rapps uuid)

add_importlibs(rapps advapi32 comctl32 gdi32 urlmon user32 shell32 shlwapi msvcrt kernel32 ntdll)
add_dependencies(rapps rappsmsg)
add_message_headers(rappsmsg.mc)
add_cd_file(TARGET rapps DESTINATION reactos/system32 FOR all)
