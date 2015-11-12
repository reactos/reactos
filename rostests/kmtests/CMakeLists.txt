
include_directories(include)

#
# subdirectories containing special-purpose drivers
#
add_subdirectory(example)
add_subdirectory(kernel32)
add_subdirectory(ntos_io)
add_subdirectory(ntos_po)
add_subdirectory(tcpip)

list(APPEND COMMON_SOURCE
    example/GuardedMemory.c
    rtl/RtlAvlTree.c
    rtl/RtlException.c
    rtl/RtlIntSafe.c
    rtl/RtlMemory.c
    rtl/RtlRegistry.c
    rtl/RtlSplayTree.c
    rtl/RtlUnicodeString.c)

#
# kmtest_drv.sys driver
#
list(APPEND KMTEST_DRV_SOURCE
    kmtest_drv/guid.c
    kmtest_drv/kmtest_drv.c
    kmtest_drv/testlist.c

    example/Example.c
    example/KernelType.c
    npfs/NpfsConnect.c
    npfs/NpfsCreate.c
    npfs/NpfsFileInfo.c
    npfs/NpfsHelpers.c
    npfs/NpfsReadWrite.c
    npfs/NpfsVolumeInfo.c
    ntos_cm/CmSecurity.c
    ntos_ex/ExCallback.c
    ntos_ex/ExDoubleList.c
    ntos_ex/ExFastMutex.c
    ntos_ex/ExHardError.c
    ntos_ex/ExInterlocked.c
    ntos_ex/ExPools.c
    ntos_ex/ExResource.c
    ntos_ex/ExSequencedList.c
    ntos_ex/ExSingleList.c
    ntos_ex/ExTimer.c
    ntos_fsrtl/FsRtlExpression.c
    ntos_fsrtl/FsRtlMcb.c
    ntos_fsrtl/FsRtlTunnel.c
    ntos_io/IoCreateFile.c
    ntos_io/IoDeviceInterface.c
    ntos_io/IoEvent.c
    ntos_io/IoFilesystem.c
    ntos_io/IoInterrupt.c
    ntos_io/IoIrp.c
    ntos_io/IoMdl.c
    ntos_ke/KeApc.c
    ntos_ke/KeDevQueue.c
    ntos_ke/KeDpc.c
    ntos_ke/KeEvent.c
    ntos_ke/KeGuardedMutex.c
    ntos_ke/KeIrql.c
    ntos_ke/KeMutex.c
    ntos_ke/KeProcessor.c
    ntos_ke/KeSpinLock.c
    ntos_ke/KeTimer.c
    ntos_mm/MmMdl.c
    ntos_mm/MmReservedMapping.c
    ntos_mm/MmSection.c
    ntos_mm/ZwAllocateVirtualMemory.c
    ntos_mm/ZwCreateSection.c
    ntos_mm/ZwMapViewOfSection.c
    ntos_ob/ObHandle.c
    ntos_ob/ObReference.c
    ntos_ob/ObType.c
    ntos_ob/ObTypes.c
    ntos_ob/ObWait.c
    ntos_ps/PsNotify.c
    ntos_se/SeHelpers.c
    ntos_se/SeInheritance.c
    ntos_se/SeQueryInfoToken.c
    ${COMMON_SOURCE}

    kmtest_drv/kmtest_drv.rc)

add_library(kmtest_drv SHARED ${KMTEST_DRV_SOURCE})
set_module_type(kmtest_drv kernelmodedriver)
target_link_libraries(kmtest_drv kmtest_printf chkstk memcmp ${PSEH_LIB})
add_importlibs(kmtest_drv ntoskrnl hal)
add_dependencies(kmtest_drv bugcodes xdk)
add_target_compile_definitions(kmtest_drv KMT_KERNEL_MODE NTDDI_VERSION=NTDDI_WS03SP1)
#add_pch(kmtest_drv include/kmt_test.h)
add_cd_file(TARGET kmtest_drv DESTINATION reactos/bin FOR all)

add_library(kmtest_printf
    kmtest_drv/printf_stubs.c
    ${REACTOS_SOURCE_DIR}/lib/sdk/crt/string/strnlen.c
    ${REACTOS_SOURCE_DIR}/lib/sdk/crt/string/wcsnlen.c
    ${REACTOS_SOURCE_DIR}/lib/sdk/crt/printf/streamout.c)
add_target_compile_definitions(kmtest_printf _LIBCNT_ _USER32_WSPRINTF wctomb=KmtWcToMb)
add_target_include_directories(kmtest_printf ${REACTOS_SOURCE_DIR}/lib/sdk/crt/include)

#
# kmtest.exe loader application
#

list(APPEND KMTEST_SOURCE
    kmtest/kmtest.c
    kmtest/service.c
    kmtest/support.c
    kmtest/testlist.c

    example/Example_user.c
    kernel32/FindFile_user.c
    ntos_io/IoCreateFile_user.c
    ntos_io/IoDeviceObject_user.c
    ntos_io/IoReadWrite_user.c
    ntos_po/PoIrp_user.c
    tcpip/TcpIp_user.c
    ${COMMON_SOURCE}

    kmtest/kmtest.rc)

add_executable(kmtest ${KMTEST_SOURCE})
set_module_type(kmtest win32cui)
target_link_libraries(kmtest ${PSEH_LIB})
add_importlibs(kmtest advapi32 ws2_32 msvcrt kernel32 ntdll)
add_target_compile_definitions(kmtest KMT_USER_MODE)
#add_pch(kmtest include/kmt_test.h)
set_target_properties(kmtest PROPERTIES OUTPUT_NAME "kmtest_")
add_cd_file(TARGET kmtest DESTINATION reactos/bin FOR all)

#
# Group targets
#
add_custom_target(kmtest_drivers)
add_dependencies(kmtest_drivers
    kmtest_drv
    example_drv
    findfile_drv
    iocreatefile_drv
    iodeviceobject_drv
    iohelper_drv
    ioreadwrite_drv
    poirp_drv
    tcpip_drv)

add_custom_target(kmtest_all)
add_dependencies(kmtest_all kmtest_drivers kmtest)
