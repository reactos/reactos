
add_library(scsiport SHARED scsiport.c stubs.c scsiport.rc)

target_link_libraries(scsiport ${CMAKE_CURRENT_SOURCE_DIR}/scsiport_i386.def)

set_module_type(scsiport kernelmodedriver)
add_importlibs(scsiport ntoskrnl hal)

add_minicd_target(scsiport reactos scsiport.sys)
add_livecd_target(scsiport reactos/system32/drivers)
