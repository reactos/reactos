
add_library(floppy SHARED
    csqrtns.c
    floppy.c
    hardware.c
    ioctl.c
    readwrite.c
    floppy.rc)

target_link_libraries(floppy csq)
add_pch(floppy precomp.h)
set_module_type(floppy kernelmodedriver)
add_importlibs(floppy hal ntoskrnl)
add_cd_file(TARGET floppy DESTINATION reactos/system32/drivers NO_CAB FOR all)
