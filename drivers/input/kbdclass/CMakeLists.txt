
add_library(kbdclass SHARED
    kbdclass.c
    misc.c
    setup.c
    kbdclass.rc)

set_module_type(kbdclass kernelmodedriver)

target_link_libraries(kbdclass ${PSEH_LIB})

add_importlibs(kbdclass ntoskrnl hal)

add_cd_file(TARGET kbdclass DESTINATION reactos/system32/drivers NO_CAB FOR all)