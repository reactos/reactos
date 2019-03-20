
include_directories(../../include)
include_directories(${REACTOS_SOURCE_DIR}/drivers/filters/fltmgr)

list(APPEND FLTMGR_TEST_DRV_SOURCE
    ../../kmtest_drv/kmtest_fsminifilter.c
    fltmgr_register.c)

add_library(fltmgrreg_drv MODULE ${FLTMGR_TEST_DRV_SOURCE})
set_module_type(fltmgrreg_drv kernelmodedriver)
target_link_libraries(fltmgrreg_drv kmtest_printf ${PSEH_LIB})
add_importlibs(fltmgrreg_drv fltmgr ntoskrnl hal)
add_target_compile_definitions(fltmgrreg_drv KMT_STANDALONE_DRIVER KMT_FILTER_DRIVER NTDDI_VERSION=NTDDI_WS03SP1)
#add_pch(example_drv ../include/kmt_test.h)
add_rostests_file(TARGET fltmgrreg_drv)
