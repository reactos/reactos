
include_directories(../include)

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
add_target_compile_definitions(iodeviceobject_drv KMT_STANDALONE_DRIVER)
#add_pch(iodeviceobject_drv ../include/kmt_test.h)
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
add_target_compile_definitions(iohelper_drv KMT_STANDALONE_DRIVER)
#add_pch(iohelper_drv ../include/kmt_test.h)
add_cd_file(TARGET iohelper_drv DESTINATION reactos/bin FOR all)

#
# IoReadWrite
#
list(APPEND IOREADWRITE_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    IoReadWrite_drv.c)

add_library(ioreadwrite_drv SHARED ${IOREADWRITE_DRV_SOURCE})
set_module_type(ioreadwrite_drv kernelmodedriver)
target_link_libraries(ioreadwrite_drv kmtest_printf ${PSEH_LIB})
add_importlibs(ioreadwrite_drv ntoskrnl hal)
add_target_compile_definitions(ioreadwrite_drv KMT_STANDALONE_DRIVER)
#add_pch(ioreadwrite_drv ../include/kmt_test.h)
add_cd_file(TARGET ioreadwrite_drv DESTINATION reactos/bin FOR all)
