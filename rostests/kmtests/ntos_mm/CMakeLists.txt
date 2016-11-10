
include_directories(../include)

#
# NtCreateSection
#
list(APPEND NTCREATESECTION_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    NtCreateSection_drv.c)

add_library(ntcreatesection_drv SHARED ${NTCREATESECTION_DRV_SOURCE})
set_module_type(ntcreatesection_drv kernelmodedriver)
target_link_libraries(ntcreatesection_drv kmtest_printf ${PSEH_LIB})
add_importlibs(ntcreatesection_drv ntoskrnl hal)
add_target_compile_definitions(ntcreatesection_drv KMT_STANDALONE_DRIVER)
#add_pch(ntcreatesection_drv ../include/kmt_test.h)
add_cd_file(TARGET ntcreatesection_drv DESTINATION reactos/bin FOR all)
