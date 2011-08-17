include_directories(
    ../include)

#
# IoDeviceObject
#
list(APPEND IODEVICEOBJECT_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    IoDeviceObject_drv.c)

add_library(iodeviceobject_drv SHARED ${IODEVICEOBJECT_DRV_SOURCE})

set_module_type(iodeviceobject_drv kernelmodedriver)
target_link_libraries(iodeviceobject_drv kmtest_printf ${PSEH_LIB})
add_importlibs(iodeviceobject_drv ntoskrnl hal)
set_property(TARGET iodeviceobject_drv PROPERTY COMPILE_DEFINITIONS KMT_STANDALONE_DRIVER)

add_cd_file(TARGET iodeviceobject_drv DESTINATION reactos/bin FOR all)

#
# IoHelper
#
list(APPEND IOHELPER_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    IoHelper_drv.c)

add_library(iohelper_drv SHARED ${IOHELPER_DRV_SOURCE})

set_module_type(iohelper_drv kernelmodedriver)
target_link_libraries(iohelper_drv kmtest_printf ${PSEH_LIB})
add_importlibs(iohelper_drv ntoskrnl hal)
set_property(TARGET iohelper_drv PROPERTY COMPILE_DEFINITIONS KMT_STANDALONE_DRIVER)

add_cd_file(TARGET iohelper_drv DESTINATION reactos/bin FOR all)
