
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
    precomp.h)

list(APPEND servman_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/res/create.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/delete.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/driver.ico
    ${CMAKE_CURRENT_SOURCE_DIR}/res/export.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/nodepends.ico
    ${CMAKE_CURRENT_SOURCE_DIR}/res/pause.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/properties.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/refresh.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/restart.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/start.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/stop.bmp
    ${CMAKE_CURRENT_SOURCE_DIR}/res/system.ico)

add_rc_deps(servman.rc ${servman_rc_deps})
add_executable(servman ${SOURCE} servman.rc)
set_module_type(servman win32gui UNICODE)
add_importlibs(servman user32 gdi32 advapi32 version comctl32 shell32 comdlg32 msvcrt kernel32)
add_pch(servman precomp.h SOURCE)
add_cd_file(TARGET servman DESTINATION reactos/system32 FOR all)
