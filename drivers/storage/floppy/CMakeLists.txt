
add_library(floppy SHARED
    csqrtns.c
    floppy.c
    hardware.c
    ioctl.c
    readwrite.c
    floppy.rc)

target_link_libraries(floppy csq)

set_module_type(floppy kernelmodedriver)
add_importlibs(floppy hal ntoskrnl)

add_minicd_target(floppy reactos floppy.sys)
add_livecd_target(floppy reactos/system32/drivers)
