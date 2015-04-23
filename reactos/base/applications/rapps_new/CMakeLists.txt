project(RAPPS_NEW)

set_cpp(WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/lib/atl)

list(APPEND SOURCE
    aboutdlg.cpp
    available.cpp
    installdlg.cpp
    installed.cpp
    listview.cpp
    loaddlg.cpp
    misc.cpp
    richedit.cpp
    settingsdlg.cpp
    splitter.cpp
    statusbar.cpp
    toolbar.cpp
    treeview.cpp
    winmain.cpp
    rapps.h)

add_executable(rapps_new ${SOURCE} rapps.rc)
set_module_type(rapps_new win32gui UNICODE)
target_link_libraries(rapps_new atlnew uuid wine)
add_importlibs(rapps_new advapi32 comctl32 gdi32 wininet user32 shell32 shlwapi ole32 msvcrt kernel32 ntdll)
add_pch(rapps_new rapps.h SOURCE)
add_dependencies(rapps_new rappsmsg_new)
add_message_headers(ANSI rappsmsg_new.mc)
add_cd_file(TARGET rapps_new DESTINATION reactos/system32 FOR all)
