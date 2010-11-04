
add_library(buslogic SHARED BusLogic958.c BusLogic958.rc)

target_link_libraries(buslogic libcntpr)

set_module_type(buslogic kernelmodedriver)
add_importlibs(buslogic scsiport)

add_minicd_target(buslogic reactos buslogic.sys)
add_livecd_target(buslogic reactos/system32/drivers)
