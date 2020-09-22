
list(APPEND SOURCE
    misc.c
    mouclass.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(mouclass MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    mouclass.rc)

set_module_type(mouclass kernelmodedriver)
target_link_libraries(mouclass ${PSEH_LIB})
add_importlibs(mouclass ntoskrnl hal)
add_pch(mouclass mouclass.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET mouclass DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(mouclass_reg.inf)
