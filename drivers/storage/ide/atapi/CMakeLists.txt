
add_library(atapi SHARED atapi.c atapi.rc)

target_link_libraries(atapi libcntpr)

set_module_type(atapi kernelmodedriver)
add_importlibs(atapi scsiport)

add_minicd_target(atapi reactos atapi.sys)
add_livecd_target(atapi reactos/system32/drivers)
