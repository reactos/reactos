
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

list(APPEND SOURCE
    chanmgr.c
    channel.c
    cmdchan.c
    concmd.c
    conmgr.c
    data.c
    dispatch.c
    init.c
    memory.c
    rawchan.c
    util.c
    vtutf8chan.c
    sacdrv.h)

add_library(sacdrv MODULE ${SOURCE} sacdrv.rc)
add_pch(sacdrv sacdrv.h SOURCE)
set_module_type(sacdrv kernelmodedriver)
add_importlibs(sacdrv ntoskrnl hal)
add_dependencies(sacdrv sacmsg)
add_cd_file(TARGET sacdrv DESTINATION reactos/system32/drivers NO_CAB FOR all)
