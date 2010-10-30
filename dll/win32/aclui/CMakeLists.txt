
set_unicode()

add_definitions(-DSUPPORT_UXTHEME)

list(APPEND SOURCE
    aclui.c
    checklist.c
    guid.c
    misc.c
    sidcache.c
    aclui.rc)

spec2def(aclui ${CMAKE_CURRENT_SOURCE_DIR}/aclui.spec ${CMAKE_CURRENT_BINARY_DIR}/aclui.def)

add_library(aclui SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/aclui_precomp.h.gch
    ${SOURCE})

set_module_type(aclui win32dll)

target_link_libraries(aclui ${CMAKE_CURRENT_BINARY_DIR}/aclui.def)
add_importlibs(aclui user32 gdi32 comctl32 ole32 oleaut32 advapi32 uxtheme kernel32 ntdll)

add_pch(aclui ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(aclui aclui_def)
add_cab_target(aclui 1)
