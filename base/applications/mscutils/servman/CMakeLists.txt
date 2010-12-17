
set_unicode()

list(APPEND SOURCE
    about.c
    control.c
    create.c
    delete.c
    dependencies_tv1.c
    dependencies_tv2.c
    export.c
    listview.c
    mainwnd.c
    misc.c
    progress.c
    propsheet.c
    propsheet_depends.c
    propsheet_general.c
    query.c
    servman.c
    start.c
    stop.c
    stop_dependencies.c
    servman.rc)

add_executable(servman ${CMAKE_CURRENT_BINARY_DIR}/servman_precomp.h.gch ${SOURCE})

add_pch(servman ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

set_module_type(servman win32gui)

add_importlibs(servman user32 gdi32 advapi32 version comctl32 shell32 comdlg32 msvcrt kernel32)

add_cab_target(servman 1)
