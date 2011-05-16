
spec2def(aclui.dll aclui.spec)
set_rc_compiler()
set_unicode()

add_definitions(-DSUPPORT_UXTHEME)

list(APPEND SOURCE
    aclui.c
    checklist.c
    guid.c
    misc.c
    sidcache.c
    aclui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/aclui.def)

add_library(aclui SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/aclui_precomp.h.gch
    ${SOURCE})

set_module_type(aclui win32dll)

add_importlib_target(aclui.spec)

add_importlibs(aclui user32 gdi32 comctl32 ole32 oleaut32 advapi32 uxtheme msvcrt kernel32 ntdll)

add_pch(aclui ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

add_cd_file(TARGET aclui DESTINATION reactos/system32 FOR all)
