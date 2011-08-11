include_directories(
    ../include)

#
# IoDriverObject
#
list(APPEND IODRIVEROBJECT_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    IoDriverObject_drv.c)

add_library(iodriverobject_drv SHARED ${IODRIVEROBJECT_DRV_SOURCE})

set_module_type(iodriverobject_drv kernelmodedriver)
target_link_libraries(iodriverobject_drv kmtest_printf ${PSEH_LIB})
add_importlibs(iodriverobject_drv ntoskrnl hal)
set_property(TARGET iodriverobject_drv PROPERTY COMPILE_DEFINITIONS KMT_STANDALONE_DRIVER)

add_cd_file(TARGET iodriverobject_drv DESTINATION reactos/bin FOR all)

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
