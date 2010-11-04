
include_directories(..)

add_library(cdrom SHARED cdrom.c findscsi.c cdrom.rc)

set_module_type(cdrom kernelmodedriver)
add_importlibs(cdrom
    class2
    scsiport
    ntoskrnl
    hal)

add_minicd_target(cdrom reactos cdrom.sys)
add_livecd_target(cdrom reactos/system32/drivers)
