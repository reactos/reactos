
set_rc_compiler()

include_directories(BEFORE .)

list(APPEND SOURCE
    about.c
    childwnd.c
    edit.c
    error.c
    find.c
    framewnd.c
    hexedit.c
    listview.c
    main.c
    regedit.c
    regproc.c
    security.c
    settings.c
    treeview.c
    regedit.rc)

add_executable(regedit ${SOURCE})

set_module_type(regedit win32gui UNICODE)

target_link_libraries(regedit uuid)

add_importlibs(regedit user32 gdi32 advapi32 ole32 shell32 comctl32 comdlg32 shlwapi msvcrt kernel32)

add_pch(regedit regedit.h)

#add_subdirectory(clb)

add_cd_file(TARGET regedit DESTINATION reactos FOR all)
