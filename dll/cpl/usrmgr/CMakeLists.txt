
set_unicode()

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(usrmgr.cpl usrmgr.spec)

add_library(usrmgr SHARED
    extra.c
    groupprops.c
    groups.c
    misc.c
    userprops.c
    users.c
    usrmgr.c
    usrmgr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/usrmgr.def)

set_module_type(usrmgr cpl)

add_importlibs(usrmgr
    advapi32
    user32
    gdi32
    comctl32
    netapi32
    msvcrt
    kernel32
    ntdll)


add_cab_target(usrmgr 1)
