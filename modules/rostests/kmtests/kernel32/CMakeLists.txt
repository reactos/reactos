
include_directories(../include)

list(APPEND KERNEL32_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    kernel32_drv.c)

add_library(kernel32_drv MODULE ${KERNEL32_DRV_SOURCE})
set_module_type(kernel32_drv kernelmodedriver)
target_link_libraries(kernel32_drv kmtest_printf ${PSEH_LIB})
add_importlibs(kernel32_drv ntoskrnl hal)
add_target_compile_definitions(kernel32_drv KMT_STANDALONE_DRIVER)
#add_pch(kernel32_drv ../include/kmt_test.h)
add_rostests_file(TARGET kernel32_drv)
