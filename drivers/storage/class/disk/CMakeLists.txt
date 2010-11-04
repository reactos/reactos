
include_directories(..)

add_library(disk SHARED disk.c disk.rc)

set_module_type(disk kernelmodedriver)
add_importlibs(disk
    class2
    scsiport
    ntoskrnl
    hal)

add_minicd_target(disk reactos disk.sys)
add_livecd_target(disk reactos/system32/drivers)
