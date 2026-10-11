
include_directories(../include)

list(APPEND MOUNTMGRVOLUME_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    MountMgrVolume_drv.c)

add_library(MountMgrVolume_drv MODULE ${MOUNTMGRVOLUME_DRV_SOURCE})
set_module_type(MountMgrVolume_drv kernelmodedriver)
target_link_libraries(MountMgrVolume_drv kmtest_printf ${PSEH_LIB})
add_importlibs(MountMgrVolume_drv ntoskrnl hal)
target_compile_definitions(MountMgrVolume_drv PRIVATE KMT_STANDALONE_DRIVER)
add_rostests_file(TARGET MountMgrVolume_drv)
