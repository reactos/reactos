
list(APPEND SOURCE
    csqrtns.c
    floppy.c
    hardware.c
    ioctl.c
    readwrite.c
    precomp.h)

add_library(floppy SHARED ${SOURCE} floppy.rc)
target_link_libraries(floppy csq)
add_pch(floppy precomp.h SOURCE)
set_module_type(floppy kernelmodedriver)
add_importlibs(floppy hal ntoskrnl)
add_cd_file(TARGET floppy DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(floppy_reg.inf)
