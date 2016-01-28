
list(APPEND SOURCE
    about.c
    affinity.c
    applpage.c
    column.c
    dbgchnl.c
    debug.c
    endproc.c
    graph.c
    graphctl.c
    optnmenu.c
    perfdata.c
    perfpage.c
    priority.c
    proclist.c
    procpage.c
    run.c
    shutdown.c
    taskmgr.c
    trayicon.c
    precomp.h)

file(GLOB taskmgr_rc_deps res/*.*)
add_rc_deps(taskmgr.rc ${taskmgr_rc_deps})
add_executable(taskmgr ${SOURCE} taskmgr.rc)
set_module_type(taskmgr win32gui UNICODE)
add_importlibs(taskmgr advapi32 user32 gdi32 shell32 shlwapi comctl32 msvcrt kernel32 ntdll)
add_pch(taskmgr precomp.h SOURCE)
add_cd_file(TARGET taskmgr DESTINATION reactos/system32 FOR all)
