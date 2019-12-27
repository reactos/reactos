
add_subdirectory(load_notifications)

include_directories($<TARGET_FILE_DIR:load_notifications>)
spec2def(ntdll_apitest.exe ntdll_apitest.spec)

list(APPEND SOURCE
    LdrEnumResources.c
    load_notifications.c
    NtAcceptConnectPort.c
    NtAllocateVirtualMemory.c
    NtApphelpCacheControl.c
    NtContinue.c
    NtCreateFile.c
    NtCreateKey.c
    NtCreateThread.c
    NtDeleteKey.c
    NtDuplicateObject.c
    NtFreeVirtualMemory.c
    NtLoadUnloadKey.c
    NtMapViewOfSection.c
    NtMutant.c
    NtOpenKey.c
    NtOpenProcessToken.c
    NtOpenThreadToken.c
    NtProtectVirtualMemory.c
    NtQueryInformationFile.c
    NtQueryInformationProcess.c
    NtQueryKey.c
    NtQuerySystemEnvironmentValue.c
    NtQuerySystemInformation.c
    NtQueryVolumeInformationFile.c
    NtReadFile.c
    NtSaveKey.c
    NtSetInformationFile.c
    NtSetValueKey.c
    NtSetVolumeInformationFile.c
    NtUnloadDriver.c
    NtWriteFile.c)

list(APPEND RTL_SOURCE
    RtlAllocateHeap.c
    RtlBitmap.c
    RtlComputePrivatizedDllName_U.c
    RtlCopyMappedMemory.c
    RtlDeleteAce.c
    RtlDetermineDosPathNameType.c
    RtlDoesFileExists.c
    RtlDosApplyFileIsolationRedirection_Ustr.c
    RtlDosPathNameToNtPathName_U.c
    RtlDosSearchPath_U.c
    RtlDosSearchPath_Ustr.c
    RtlFirstFreeAce.c
    RtlGenerate8dot3Name.c
    RtlGetFullPathName_U.c
    RtlGetFullPathName_Ustr.c
    RtlGetFullPathName_UstrEx.c
    RtlGetLengthWithoutTrailingPathSeperators.c
    RtlGetLongestNtPathLength.c
    RtlHandle.c
    RtlImageRvaToVa.c
    RtlIsNameLegalDOS8Dot3.c
    RtlMemoryStream.c
    RtlNtPathNameToDosPathName.c
    RtlpEnsureBufferSize.c
    RtlQueryTimeZoneInfo.c
    RtlReAllocateHeap.c
    RtlUnicodeStringToAnsiString.c
    RtlUpcaseUnicodeStringToCountedOemString.c
    RtlValidateUnicodeString.c
    StackOverflow.c
    SystemInfo.c
    Timer.c
    precomp.h)

if(ARCH STREQUAL "i386")
    add_asm_files(ntdll_apitest_asm
        i386/NtContinue.S)
elseif(ARCH STREQUAL "amd64")
    list(APPEND RTL_SOURCE
        amd64/RtlCaptureContext.c
        amd64/RtlVirtualUnwind.c)
    add_asm_files(ntdll_apitest_asm
        amd64/UnwindStubs.S
        amd64/RtlCaptureContext.S)
endif()

add_rc_deps(testdata.rc ${CMAKE_CURRENT_BINARY_DIR}/load_notifications/load_notifications.dll)
add_executable(ntdll_apitest
    ${SOURCE}
    ${RTL_SOURCE}
    ${ntdll_apitest_asm}
    testdata.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll_apitest.def
    testlist.c)
target_link_libraries(ntdll_apitest wine uuid ${PSEH_LIB})
set_module_type(ntdll_apitest win32cui)
add_importlibs(ntdll_apitest msvcrt advapi32 kernel32 ntdll)
#add_pch(ntdll_apitest precomp.h SOURCE)

if(NOT MSVC)
    set_source_files_properties(RtlGetFullPathName_UstrEx.c PROPERTIES COMPILE_FLAGS "-Wno-format")

    # Avoid "universal character names are only valid in C++ and C99" error.
    set_property(TARGET ntdll_apitest PROPERTY C_STANDARD 99)
endif()

add_rostests_file(TARGET ntdll_apitest)
add_rostests_file(TARGET ntdll_apitest SUBDIR testdata)
# These are empty files
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/ntdll_apitest.exe.local" SUBDIR testdata)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/shell32.dll" SUBDIR testdata)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/test.dll" SUBDIR testdata)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/ntdlltest.manifest" SUBDIR testdata)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/fil1.txt" SUBDIR testdata)

# Convenience target for running from IDE
if(MSVC_IDE)
    set(NTDLL_APITEST_DATADIR $<TARGET_FILE_DIR:ntdll_apitest>/testdata/)
    add_custom_command(TARGET ntdll_apitest
                       POST_BUILD
                       COMMAND "${CMAKE_COMMAND}" -E make_directory ${NTDLL_APITEST_DATADIR}
                       COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:ntdll_apitest> ${NTDLL_APITEST_DATADIR}
                       COMMAND "${CMAKE_COMMAND}" -E copy ntdll_apitest.exe.local ${NTDLL_APITEST_DATADIR}
                       COMMAND "${CMAKE_COMMAND}" -E copy shell32.dll ${NTDLL_APITEST_DATADIR}
                       COMMAND "${CMAKE_COMMAND}" -E copy test.dll ${NTDLL_APITEST_DATADIR}
                       COMMAND "${CMAKE_COMMAND}" -E copy ntdlltest.manifest ${NTDLL_APITEST_DATADIR}
                       COMMAND "${CMAKE_COMMAND}" -E copy fil1.txt ${NTDLL_APITEST_DATADIR}
                       WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# RTL tests with static linkage
#add_executable(rtl_apitest ${RTL_SOURCE} ${ntdll_apitest_asm} testlist.c rtl/libsupp.c)
#add_target_compile_flags(rtl_apitest "-D_RTL_TEST -D_NTSYSTEM_")
#target_link_libraries(rtl_apitest wine uuid ${PSEH_LIB} rtl)
#set_module_type(rtl_apitest win32cui)
#add_importlibs(rtl_apitest msvcrt advapi32 kernel32 ntdll)

