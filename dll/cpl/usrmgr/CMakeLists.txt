
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(usrmgr.cpl usrmgr.spec)

list(APPEND SOURCE
    extra.c
    groupprops.c
    groups.c
    misc.c
    userprops.c
    users.c
    usrmgr.c
    usrmgr.h)

file(GLOB usrmgr_rc_deps resources/*.*)
add_rc_deps(usrmgr.rc ${usrmgr_rc_deps})

add_library(usrmgr SHARED
    ${SOURCE}
    usrmgr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/usrmgr.def)

set_module_type(usrmgr cpl UNICODE)
add_importlibs(usrmgr advapi32 user32 gdi32 comctl32 netapi32 msvcrt kernel32 ntdll)
add_pch(usrmgr usrmgr.h SOURCE)
add_cd_file(TARGET usrmgr DESTINATION reactos/system32 FOR all)
