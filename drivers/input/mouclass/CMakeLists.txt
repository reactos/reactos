
list(APPEND SOURCE
    misc.c
    mouclass.c
    mouclass.h)

add_library(mouclass SHARED
    ${SOURCE}
    guid.c
    mouclass.rc)

set_module_type(mouclass kernelmodedriver)
target_link_libraries(mouclass ${PSEH_LIB})
add_importlibs(mouclass ntoskrnl hal)
add_pch(mouclass mouclass.h SOURCE)
add_cd_file(TARGET mouclass DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(mouclass_reg.inf)
