
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

add_library(kmtest SHARED ${KMTEST_SOURCE})

set_module_type(kmtest kernelmodedriver)
target_link_libraries(kmtest ${PSEH_LIB})
add_importlibs(kmtest ntoskrnl hal)

add_minicd_target(kmtest reactos kmtest.sys)
add_livecd_target(kmtest reactos/system32/drivers)


add_library(kmtestassist SHARED kmtestassist.c)

set_module_type(kmtestassist kernelmodedriver)
add_importlibs(kmtestassist ntoskrnl hal)

add_minicd_target(kmtestassist reactos kmtestassist.sys)
add_livecd_target(kmtestassist reactos/system32/drivers)
