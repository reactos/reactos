
include_directories(../include)

#
# PoIrp
#
list(APPEND POIRP_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    PoIrp_drv.c)

add_library(poirp_drv SHARED ${POIRP_DRV_SOURCE})
set_module_type(poirp_drv kernelmodedriver)
target_link_libraries(poirp_drv kmtest_printf ${PSEH_LIB})
add_importlibs(poirp_drv ntoskrnl hal)
add_target_compile_definitions(poirp_drv KMT_STANDALONE_DRIVER)
#add_pch(poirp_drv ../include/kmt_test.h)
add_cd_file(TARGET poirp_drv DESTINATION reactos/bin FOR all)
