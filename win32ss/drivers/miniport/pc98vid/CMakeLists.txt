
list(APPEND SOURCE
    hardware.c
    ioctl.c
    pc98vid.c
    pc98vid.h)

# Actual binary filename is vga.sys
add_library(vga MODULE ${SOURCE} pc98vid.rc)

set_module_type(vga kernelmodedriver)
add_pch(vga pc98vid.h SOURCE)
add_importlibs(vga ntoskrnl videoprt)
add_cd_file(TARGET vga DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(pc98vid_reg.inf)
add_driver_inf(vga pc98disp.inf)
