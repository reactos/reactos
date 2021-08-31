
add_subdirectory(load_notifications)

include_directories($<TARGET_FILE_DIR:load_notifications>)
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)
spec2def(ntdll_apitest.exe ntdll_apitest.spec)

list(APPEND SOURCE
    LdrEnumResources.c
    load_notifications.c
    locale.c
    NtAcceptConnectPort.c
    NtAdjustPrivilegesToken.c
    NtAllocateVirtualMemory.c
    NtApphelpCacheControl.c
    NtCompareTokens.c
    NtContinue.c
    NtCreateFile.c
    NtCreateKey.c
    NtCreateThread.c
    NtDeleteKey.c
    NtDuplicateObject.c
    NtDuplicateToken.c
    NtFreeVirtualMemory.c
    NtImpersonateAnonymousToken.c
    NtLoadUnloadKey.c
    NtMapViewOfSection.c
    NtMutant.c
    NtOpenKey.c
    NtOpenProcessToken.c
    NtOpenThreadToken.c
    NtProtectVirtualMemory.c
    NtQueryInformationFile.c
    NtQueryInformationProcess.c
    NtQueryInformationThread.c
    NtQueryKey.c
    NtQuerySystemEnvironmentValue.c
    NtQuerySystemInformation.c
    NtQueryValueKey.c
    NtQueryVolumeInformationFile.c
    NtReadFile.c
    NtSaveKey.c
    NtSetInformationFile.c
    NtSetInformationProcess.c
    NtSetInformationThread.c
    NtSetValueKey.c
    NtSetVolumeInformationFile.c
    NtUnloadDriver.c
    NtWriteFile.c
    probelib.c
    RtlAllocateHeap.c
    RtlBitmap.c
    RtlComputePrivatizedDllName_U.c
    RtlCopyMappedMemory.c
    RtlDebugInformation.c
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
    RtlGetLengthWithoutLastFullDosOrNtPathElement.c
    RtlGetLengthWithoutTrailingPathSeperators.c
    RtlGetLongestNtPathLength.c
    RtlGetNtProductType.c
    RtlGetUnloadEventTrace.c
    RtlHandle.c
    RtlImageDirectoryEntryToData.c
    RtlImageRvaToVa.c
    RtlIsNameLegalDOS8Dot3.c
    RtlMemoryStream.c
    RtlMultipleAllocateHeap.c
    RtlNtPathNameToDosPathName.c
    RtlpApplyLengthFunction.c
    RtlpEnsureBufferSize.c
    RtlQueryTimeZoneInfo.c
    RtlReAllocateHeap.c
    RtlUnicodeStringToAnsiString.c
    RtlUnicodeStringToCountedOemString.c
    RtlUnicodeToOemN.c
    RtlUpcaseUnicodeStringToCountedOemString.c
    RtlValidateUnicodeString.c
    RtlxUnicodeStringToAnsiSize.c
    RtlxUnicodeStringToOemSize.c
    StackOverflow.c
    SystemInfo.c
    Timer.c)

if(ARCH STREQUAL "i386")
    add_asm_files(ntdll_apitest_asm i386/NtContinue.S)
endif()

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_rc_deps(testdata.rc ${CMAKE_CURRENT_BINARY_DIR}/load_notifications/load_notifications.dll)

add_executable(ntdll_apitest
    ${SOURCE}
    ${ntdll_apitest_asm}
    ${PCH_SKIP_SOURCE}
    testdata.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll_apitest.def)

target_link_libraries(ntdll_apitest wine uuid ${PSEH_LIB})
set_module_type(ntdll_apitest win32cui)
add_importlibs(ntdll_apitest msvcrt advapi32 kernel32 ntdll)
add_pch(ntdll_apitest precomp.h "${PCH_SKIP_SOURCE}")
add_dependencies(ntdll_apitest load_notifications)

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
