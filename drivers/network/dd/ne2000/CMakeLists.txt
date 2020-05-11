
include_directories(BEFORE include)

list(APPEND SOURCE
    ne2000/8390.c
    ne2000/main.c
    include/ne2000.h)

add_library(ne2000 MODULE ${SOURCE} ne2000.rc)
add_pch(ne2000 include/ne2000.h SOURCE)
set_module_type(ne2000 kernelmodedriver)
add_importlibs(ne2000 ndis ntoskrnl hal)
add_cd_file(TARGET ne2000 DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(ne2000 netne2000.inf netrtl8029.inf)
