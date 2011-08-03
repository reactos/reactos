include_directories(
    ../include)

list(APPEND EXAMPLE_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    IoDriverObject_drv.c)

add_library(iodriverobject_drv SHARED ${EXAMPLE_DRV_SOURCE})

set_module_type(iodriverobject_drv kernelmodedriver)
target_link_libraries(iodriverobject_drv kmtest_printf ${PSEH_LIB})
add_importlibs(iodriverobject_drv ntoskrnl hal)
set_property(TARGET iodriverobject_drv PROPERTY COMPILE_DEFINITIONS KMT_STANDALONE_DRIVER)

add_cd_file(TARGET iodriverobject_drv DESTINATION reactos/bin FOR all)
