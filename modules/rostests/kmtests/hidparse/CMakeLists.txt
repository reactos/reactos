
include_directories(../include)

#
# HidP
#
list(APPEND HIDP_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    HidP_drv.c
    HidPDescription.c)

add_library(hidp_drv MODULE ${HIDP_DRV_SOURCE})
set_module_type(hidp_drv kernelmodedriver)
target_link_libraries(hidp_drv kmtest_printf ${PSEH_LIB})
add_importlibs(hidp_drv hidparse ntoskrnl hal)
add_target_compile_definitions(hidp_drv KMT_STANDALONE_DRIVER)
#add_pch(hidp_drv ../include/kmt_test.h)
add_rostests_file(TARGET hidp_drv)
