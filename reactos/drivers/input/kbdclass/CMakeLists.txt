
list(APPEND SOURCE
    kbdclass.c
    misc.c
    kbdclass.h)

add_library(kbdclass SHARED
    ${SOURCE}
    guid.c
    kbdclass.rc)

set_module_type(kbdclass kernelmodedriver)
add_pch(kbdclass kbdclass.h SOURCE)
target_link_libraries(kbdclass ${PSEH_LIB})
add_importlibs(kbdclass ntoskrnl hal)
add_cd_file(TARGET kbdclass DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(kbdclass_reg.inf)
