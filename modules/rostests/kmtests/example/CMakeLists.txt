
include_directories(../include)

list(APPEND EXAMPLE_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    Example_drv.c)

add_library(example_drv MODULE ${EXAMPLE_DRV_SOURCE})
set_module_type(example_drv kernelmodedriver)
target_link_libraries(example_drv kmtest_printf ${PSEH_LIB})
add_importlibs(example_drv ntoskrnl hal)
add_target_compile_definitions(example_drv KMT_STANDALONE_DRIVER)
#add_pch(example_drv ../include/kmt_test.h)
add_rostests_file(TARGET example_drv)
