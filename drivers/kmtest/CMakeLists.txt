
list(APPEND KMTEST_SOURCE
    kmtest.c
    deviface_test.c
    drvobj_test.c
    devobj_test.c
    reghelper.c
    ntos_ex.c
    ntos_io.c
    ntos_ke.c
    ntos_ob.c
    ntos_pools.c
    ntos_fsrtl.c
    kmtest.rc)

add_library(kmtest_old SHARED ${KMTEST_SOURCE})

set_module_type(kmtest_old kernelmodedriver)
target_link_libraries(kmtest_old ${PSEH_LIB})
add_importlibs(kmtest_old ntoskrnl hal)

add_cd_file(TARGET kmtest_old DESTINATION reactos/system32/drivers FOR all)


add_library(kmtestassist SHARED kmtestassist.c)

set_module_type(kmtestassist kernelmodedriver)
add_importlibs(kmtestassist ntoskrnl hal)

add_cd_file(TARGET kmtestassist DESTINATION reactos/system32/drivers FOR all)
