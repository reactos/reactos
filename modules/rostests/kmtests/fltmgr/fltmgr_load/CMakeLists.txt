
include_directories(../../include)

list(APPEND FLTMGR_TEST_DRV_SOURCE
    ../../kmtest_drv/kmtest_fsminifilter.c
    fltmgr_load.c)

add_library(FltMgrLoad_drv MODULE ${FLTMGR_TEST_DRV_SOURCE})
set_module_type(FltMgrLoad_drv kernelmodedriver)
target_link_libraries(FltMgrLoad_drv kmtest_printf ${PSEH_LIB})
add_importlibs(FltMgrLoad_drv fltmgr ntoskrnl hal)
add_target_compile_definitions(FltMgrLoad_drv KMT_STANDALONE_DRIVER KMT_FILTER_DRIVER NTDDI_VERSION=NTDDI_WS03SP1)
#add_pch(example_drv ../include/kmt_test.h)
add_rostests_file(TARGET FltMgrLoad_drv)
