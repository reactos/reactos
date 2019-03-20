
include_directories(../../include)

list(APPEND FLTMGR_TEST_DRV_SOURCE
    ../../kmtest_drv/kmtest_fsminifilter.c
    fltmgr_create.c)

add_library(fltmgr_create MODULE ${FLTMGR_TEST_DRV_SOURCE})
set_module_type(fltmgr_create kernelmodedriver)
target_link_libraries(fltmgr_create kmtest_printf ${PSEH_LIB})
add_importlibs(fltmgr_create fltmgr ntoskrnl hal)
add_target_compile_definitions(fltmgr_create KMT_STANDALONE_DRIVER KMT_FILTER_DRIVER NTDDI_VERSION=NTDDI_WS03SP1)
#add_pch(example_drv ../include/kmt_test.h)
add_rostests_file(TARGET fltmgr_create)
