
list(APPEND SOURCE
    hardware.c
    ioctl.c
    pc98vid.c
    pc98vid.h)

add_library(pc98vid MODULE ${SOURCE} pc98vid.rc)

set_module_type(pc98vid kernelmodedriver)
# Actual binary filename is vga.sys
set_target_properties(pc98vid PROPERTIES OUTPUT_NAME "vga" PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
add_pch(pc98vid pc98vid.h SOURCE)
add_importlibs(pc98vid ntoskrnl videoprt)

if(SARCH STREQUAL "pc98")
    add_cd_file(TARGET pc98vid DESTINATION reactos/system32/drivers FOR all)
    add_registry_inf(pc98vid_reg.inf)
    add_driver_inf(pc98vid pc98disp.inf)
endif()
