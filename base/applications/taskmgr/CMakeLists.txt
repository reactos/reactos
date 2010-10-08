
set_unicode()

list(APPEND SOURCE
    about.c
    affinity.c
    applpage.c
    column.c
    dbgchnl.c
    debug.c
    endproc.c
    graph.c
    optnmenu.c
    perfdata.c
    perfpage.c
    priority.c
    procpage.c
    proclist.c
    run.c
    trayicon.c
    taskmgr.c
    graphctl.c
    taskmgr.rc)

add_executable(taskmgr ${CMAKE_CURRENT_BINARY_DIR}/taskmgr_precomp.h.gch ${SOURCE})

add_pch(taskmgr ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

set_module_type(taskmgr win32gui)

add_importlibs(taskmgr advapi32 user32 gdi32 shell32 comctl32 msvcrt kernel32 ntdll)
