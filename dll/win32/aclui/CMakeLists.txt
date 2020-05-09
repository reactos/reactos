
spec2def(aclui.dll aclui.spec)
add_definitions(-DSUPPORT_UXTHEME)

list(APPEND SOURCE
    aclui.c
    checklist.c
    misc.c
    sidcache.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(aclui MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    aclui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/aclui.def)

set_module_type(aclui win32dll UNICODE)
add_importlibs(aclui user32 gdi32 comctl32 ole32 oleaut32 advapi32 uxtheme msvcrt kernel32 ntdll)
add_pch(aclui precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET aclui DESTINATION reactos/system32 FOR all)
