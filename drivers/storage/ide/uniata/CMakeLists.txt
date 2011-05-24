
include_directories(inc)

set_cpp()
set_rc_compiler()

add_library(uniata SHARED
    id_ata.cpp
    id_badblock.cpp
    id_dma.cpp
    id_init.cpp
    id_probe.cpp
    id_queue.cpp
    id_sata.cpp
    idedma.rc
    ros_glue/ros_glue.cpp)

set_module_type(uniata kernelmodedriver)
add_importlibs(uniata scsiport ntoskrnl hal)

add_cd_file(TARGET uniata DESTINATION reactos/system32/drivers NO_CAB FOR all)