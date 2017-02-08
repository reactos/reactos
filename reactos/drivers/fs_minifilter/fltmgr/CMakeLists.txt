
list(APPEND SOURCE
    Interface.c
    Lib.c
    fltmgr.h)

add_library(rosfltmgr SHARED ${SOURCE} fltmgr.rc)
set_module_type(rosfltmgr kernelmodedriver)
target_link_libraries(rosfltmgr ${PSEH_LIB})
add_importlibs(rosfltmgr ntoskrnl hal)
add_pch(rosfltmgr fltmgr.h SOURCE)
add_cd_file(TARGET rosfltmgr DESTINATION reactos/system32/drivers NO_CAB FOR all)
