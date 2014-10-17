
include_directories(../include)

list(APPEND FINDFILE_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    FindFile_drv.c)

add_library(findfile_drv SHARED ${FINDFILE_DRV_SOURCE})
set_module_type(findfile_drv kernelmodedriver)
target_link_libraries(findfile_drv kmtest_printf ${PSEH_LIB})
add_importlibs(findfile_drv ntoskrnl hal)
add_target_compile_definitions(findfile_drv KMT_STANDALONE_DRIVER)
#add_pch(findfile_drv ../include/kmt_test.h)
add_cd_file(TARGET findfile_drv DESTINATION reactos/bin FOR all)
