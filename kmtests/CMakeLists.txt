include_directories(
    include)

#
# subdirectories containing special-purpose drivers
#
add_subdirectory(example)
add_subdirectory(ntos_io)

#
# kmtest_drv.sys driver
#
list(APPEND KMTEST_DRV_SOURCE
    kmtest_drv/kmtest_drv.c
    kmtest_drv/testlist.c

    example/Example.c
    example/KernelType.c
    ntos_ex/ExDoubleList.c
    ntos_ex/ExFastMutex.c
    ntos_ex/ExHardError.c
    ntos_ex/ExInterlocked.c
    ntos_ex/ExPools.c
    ntos_ex/ExResource.c
    ntos_ex/ExSingleList.c
    ntos_ex/ExTimer.c
    ntos_fsrtl/FsRtlExpression.c
    ntos_io/IoDeviceInterface.c
    ntos_io/IoIrp.c
    ntos_io/IoMdl.c
    ntos_ke/KeApc.c
    ntos_ke/KeDpc.c
    ntos_ke/KeEvent.c
    ntos_ke/KeIrql.c
    ntos_ke/KeProcessor.c
    ntos_ke/KeSpinLock.c
    ntos_ob/ObCreate.c
    rtl/RtlAvlTree.c
    rtl/RtlMemory.c
    rtl/RtlSplayTree.c

    kmtest_drv/kmtest_drv.rc)

add_library(kmtest_drv SHARED ${KMTEST_DRV_SOURCE})

set_module_type(kmtest_drv kernelmodedriver)
target_link_libraries(kmtest_drv kmtest_printf ${PSEH_LIB})
add_importlibs(kmtest_drv ntoskrnl hal)
set_property(TARGET kmtest_drv PROPERTY COMPILE_DEFINITIONS KMT_KERNEL_MODE NTDDI_VERSION=NTDDI_WS03SP1)

add_cd_file(TARGET kmtest_drv DESTINATION reactos/bin FOR all)

add_library(kmtest_printf
    kmtest_drv/printf_stubs.c
    ${REACTOS_SOURCE_DIR}/lib/sdk/crt/printf/streamout.c)
set_property(TARGET kmtest_printf PROPERTY COMPILE_DEFINITIONS _LIBCNT_ _USER32_WSPRINTF wctomb=KmtWcToMb)
set_property(TARGET kmtest_printf PROPERTY INCLUDE_DIRECTORIES ${REACTOS_SOURCE_DIR}/lib/sdk/crt/include)

#
# kmtest.exe loader application
#
set_rc_compiler()

list(APPEND KMTEST_SOURCE
    kmtest/kmtest.c
    kmtest/service.c
    kmtest/support.c
    kmtest/testlist.c

    example/Example_user.c
    ntos_io/IoDriverObject_user.c
    rtl/RtlAvlTree.c
    rtl/RtlMemory.c
    rtl/RtlSplayTree.c

    kmtest/kmtest.rc)

add_executable(kmtest ${KMTEST_SOURCE})
set_module_type(kmtest win32cui)
target_link_libraries(kmtest ${PSEH_LIB})
add_importlibs(kmtest advapi32 msvcrt kernel32 ntdll)
set_property(TARGET kmtest PROPERTY COMPILE_DEFINITIONS KMT_USER_MODE)

add_cd_file(TARGET kmtest DESTINATION reactos/bin FOR all)

#
# Group targets
#
add_custom_target(kmtest_drivers)
add_dependencies(kmtest_drivers kmtest_drv example_drv iodriverobject_drv)

add_custom_target(kmtest_all)
add_dependencies(kmtest_all kmtest_drivers kmtest)
