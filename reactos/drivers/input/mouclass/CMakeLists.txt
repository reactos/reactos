
add_library(mouclass SHARED
    misc.c
    mouclass.c
    mouclass.rc)

set_module_type(mouclass kernelmodedriver)
add_pch(mouclass mouclass.h)
target_link_libraries(mouclass ${PSEH_LIB})
add_importlibs(mouclass ntoskrnl hal)
add_cd_file(TARGET mouclass DESTINATION reactos/system32/drivers FOR all)
