
include_directories(../include)

list(APPEND TCPIP_TEST_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    tdi.c
    TcpIp_drv.c)

add_library(tcpip_drv SHARED ${TCPIP_TEST_DRV_SOURCE})
set_module_type(tcpip_drv kernelmodedriver)
target_link_libraries(tcpip_drv kmtest_printf ${PSEH_LIB})
add_importlibs(tcpip_drv ntoskrnl hal)
add_target_compile_definitions(tcpip_drv KMT_STANDALONE_DRIVER)
#add_pch(example_drv ../include/kmt_test.h)
add_cd_file(TARGET tcpip_drv DESTINATION reactos/bin FOR all)
