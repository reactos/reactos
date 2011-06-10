include_directories(
    include)

#
# subdirectories containing special-purpose drivers
#
#add_subdirectory(something)

#
# kmtest_drv.sys driver
#
list(APPEND KMTEST_DRV_SOURCE
    kmtest_drv/kmtest_drv.c
    kmtest_drv/log.c
    kmtest_drv/testlist.c

    kmtest_drv/kmtest_drv.rc)

add_library(kmtest_drv SHARED ${KMTEST_DRV_SOURCE})

set_module_type(kmtest_drv kernelmodedriver)
target_link_libraries(kmtest_drv ${PSEH_LIB})
add_importlibs(kmtest_drv ntoskrnl hal)

add_cd_file(TARGET kmtest_drv DESTINATION reactos/system32/drivers FOR all)

#
# kmtest.exe loader application
#
set_rc_compiler()

add_definitions(-D_DLL -D__USE_CRTIMP)

list(APPEND KMTEST_SOURCE
    kmtest/kmtest.c
    kmtest/service.c
    kmtest/kmtest.rc)

add_executable(kmtest ${KMTEST_SOURCE})
set_module_type(kmtest win32cui)
add_importlibs(kmtest advapi32 msvcrt kernel32)

add_cd_file(TARGET kmtest DESTINATION reactos/bin FOR all)
