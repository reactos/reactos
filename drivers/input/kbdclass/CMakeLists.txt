
list(APPEND SOURCE
    kbdclass.c
    misc.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(kbdclass MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    kbdclass.rc)

set_module_type(kbdclass kernelmodedriver)
add_pch(kbdclass kbdclass.h "${PCH_SKIP_SOURCE}")
target_link_libraries(kbdclass ${PSEH_LIB})
add_importlibs(kbdclass ntoskrnl hal)
add_cd_file(TARGET kbdclass DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(kbdclass_reg.inf)
