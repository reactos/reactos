
list(APPEND SOURCE
    database.c
    device.c
    mountmgr.c
    notify.c
    point.c
    symlink.c
    uniqueid.c
    mntmgr.h)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(mountmgr MODULE ${SOURCE} ${PCH_SKIP_SOURCE} mountmgr.rc)
target_link_libraries(mountmgr wdmguid ioevent memcmp)
set_module_type(mountmgr kernelmodedriver)
add_importlibs(mountmgr ntoskrnl hal)
add_pch(mountmgr mntmgr.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET mountmgr DESTINATION reactos/system32/drivers NO_CAB FOR all)
