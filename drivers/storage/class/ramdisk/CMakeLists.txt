
include_directories(..)

add_library(ramdisk SHARED ramdisk.c ramdisk.rc)

set_module_type(ramdisk kernelmodedriver)
add_importlibs(ramdisk
    ntoskrnl
    hal)

add_minicd_target(ramdisk reactos ramdisk.sys)
add_livecd_target(ramdisk reactos/system32/drivers)
