
list(APPEND SOURCE
    database.c
    device.c
    mountmgr.c
    notify.c
    point.c
    symlink.c
    uniqueid.c
    mntmgr.h)

add_library(mountmgr MODULE ${SOURCE} mountmgr.rc)
target_link_libraries(mountmgr wdmguid ioevent memcmp)
set_module_type(mountmgr kernelmodedriver)
add_importlibs(mountmgr ntoskrnl hal)
add_pch(mountmgr mntmgr.h SOURCE)
add_cd_file(TARGET mountmgr DESTINATION reactos/system32/drivers NO_CAB FOR all)
