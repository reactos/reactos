
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
     vtutf8chan.c)

add_library(sacdrv SHARED ${SOURCE} sacdrv.rc)
set_module_type(sacdrv kernelmodedriver)
add_importlibs(sacdrv ntoskrnl hal)
add_dependencies(sacdrv sacmsg)
add_cd_file(TARGET sacdrv DESTINATION reactos/system32/drivers NO_CAB FOR all)
