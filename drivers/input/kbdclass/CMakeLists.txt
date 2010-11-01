


add_library(kbdclass SHARED
    kbdclass.c
    misc.c
    setup.c
    kbdclass.rc)

set_module_type(kbdclass kernelmodedriver)

target_link_libraries(kbdclass ${PSEH_LIB})

add_importlibs(kbdclass ntoskrnl hal)

add_minicd_target(kbdclass reactos kbdclass.sys)
add_livecd_target(kbdclass reactos/system32/drivers)