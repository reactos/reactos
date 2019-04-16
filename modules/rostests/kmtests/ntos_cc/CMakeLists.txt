
include_directories(../include)

#
# CcCopyRead
#
list(APPEND CCCOPYREAD_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    CcCopyRead_drv.c)

add_library(cccopyread_drv MODULE ${CCCOPYREAD_DRV_SOURCE})
set_module_type(cccopyread_drv kernelmodedriver)
target_link_libraries(cccopyread_drv kmtest_printf ${PSEH_LIB})
add_importlibs(cccopyread_drv ntoskrnl hal)
add_target_compile_definitions(cccopyread_drv KMT_STANDALONE_DRIVER)
#add_pch(cccopyread_drv ../include/kmt_test.h)
add_rostests_file(TARGET cccopyread_drv)

#
# CcCopyWrite
#
list(APPEND CCCOPYWRITE_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    CcCopyWrite_drv.c)

add_library(cccopywrite_drv MODULE ${CCCOPYWRITE_DRV_SOURCE})
set_module_type(cccopywrite_drv kernelmodedriver)
target_link_libraries(cccopywrite_drv kmtest_printf ${PSEH_LIB})
add_importlibs(cccopywrite_drv ntoskrnl hal)
add_target_compile_definitions(cccopywrite_drv KMT_STANDALONE_DRIVER)
#add_pch(cccopyread_drv ../include/kmt_test.h)
add_rostests_file(TARGET cccopywrite_drv)

#
# CcMapData
#
list(APPEND CCMAPDATA_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    CcMapData_drv.c)

add_library(ccmapdata_drv MODULE ${CCMAPDATA_DRV_SOURCE})
set_module_type(ccmapdata_drv kernelmodedriver)
target_link_libraries(ccmapdata_drv kmtest_printf ${PSEH_LIB})
add_importlibs(ccmapdata_drv ntoskrnl hal)
add_target_compile_definitions(ccmapdata_drv KMT_STANDALONE_DRIVER)
#add_pch(ccmapdata_drv ../include/kmt_test.h)
add_rostests_file(TARGET ccmapdata_drv)

#
# CcPinMappedData
#
list(APPEND CCPINMAPPEDDATA_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    CcPinMappedData_drv.c)

add_library(ccpinmappeddata_drv MODULE ${CCPINMAPPEDDATA_DRV_SOURCE})
set_module_type(ccpinmappeddata_drv kernelmodedriver)
target_link_libraries(ccpinmappeddata_drv kmtest_printf ${PSEH_LIB})
add_importlibs(ccpinmappeddata_drv ntoskrnl hal)
add_target_compile_definitions(ccpinmappeddata_drv KMT_STANDALONE_DRIVER)
#add_pch(ccpinmappeddata_drv ../include/kmt_test.h)
add_rostests_file(TARGET ccpinmappeddata_drv)

#
# CcPinRead
#
list(APPEND CCPINREAD_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    CcPinRead_drv.c)

add_library(ccpinread_drv MODULE ${CCPINREAD_DRV_SOURCE})
set_module_type(ccpinread_drv kernelmodedriver)
target_link_libraries(ccpinread_drv kmtest_printf ${PSEH_LIB})
add_importlibs(ccpinread_drv ntoskrnl hal)
add_target_compile_definitions(ccpinread_drv KMT_STANDALONE_DRIVER)
#add_pch(ccmapdata_drv ../include/kmt_test.h)
add_rostests_file(TARGET ccpinread_drv)

#
# CcSetFileSizes
#
list(APPEND CCSETFILESIZES_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    CcSetFileSizes_drv.c)

add_library(ccsetfilesizes_drv MODULE ${CCSETFILESIZES_DRV_SOURCE})
set_module_type(ccsetfilesizes_drv kernelmodedriver)
target_link_libraries(ccsetfilesizes_drv kmtest_printf ${PSEH_LIB})
add_importlibs(ccsetfilesizes_drv ntoskrnl hal)
add_target_compile_definitions(ccsetfilesizes_drv KMT_STANDALONE_DRIVER)
#add_pch(ccsetfilesizes_drv ../include/kmt_test.h)
add_rostests_file(TARGET ccsetfilesizes_drv)
