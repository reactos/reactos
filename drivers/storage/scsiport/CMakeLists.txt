
spec2def(scsiport.sys scsiport.spec)

list(APPEND SOURCE
    scsiport.c
    stubs.c
    scsiport.rc
    ${CMAKE_CURRENT_BINARY_DIR}/scsiport.def)
    
add_library(scsiport SHARED ${SOURCE})

set_module_type(scsiport kernelmodedriver)

add_importlibs(scsiport ntoskrnl hal)
add_importlib_target(scsiport.spec)

add_minicd_target(scsiport reactos scsiport.sys)
add_livecd_target(scsiport reactos/system32/drivers)
