
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
    NtWriteFile.c
    StackOverflow.c
    SystemInfo.c
    Timer.c
    precomp.h)

list(APPEND RTL_SOURCE
    ../rtl/RtlAllocateHeap.c
    ../rtl/RtlBitmap.c
    ../rtl/RtlComputePrivatizedDllName_U.c
    ../rtl/RtlCopyMappedMemory.c
    ../rtl/RtlDeleteAce.c
    ../rtl/RtlDetermineDosPathNameType.c
    ../rtl/RtlDoesFileExists.c
    ../rtl/RtlDosApplyFileIsolationRedirection_Ustr.c
    ../rtl/RtlDosPathNameToNtPathName_U.c
    ../rtl/RtlDosSearchPath_U.c
    ../rtl/RtlDosSearchPath_Ustr.c
    ../rtl/RtlFirstFreeAce.c
    ../rtl/RtlGenerate8dot3Name.c
    ../rtl/RtlGetFullPathName_U.c
    ../rtl/RtlGetFullPathName_Ustr.c
    ../rtl/RtlGetFullPathName_UstrEx.c
    ../rtl/RtlGetLengthWithoutTrailingPathSeperators.c
    ../rtl/RtlGetLongestNtPathLength.c
    ../rtl/RtlHandle.c
    ../rtl/RtlImageRvaToVa.c
    ../rtl/RtlIsNameLegalDOS8Dot3.c
    ../rtl/RtlMemoryStream.c
    ../rtl/RtlNtPathNameToDosPathName.c
    ../rtl/RtlpEnsureBufferSize.c
    ../rtl/RtlQueryTimeZoneInfo.c
    ../rtl/RtlReAllocateHeap.c
    ../rtl/RtlUnicodeStringToAnsiString.c
    ../rtl/RtlUpcaseUnicodeStringToCountedOemString.c)

if(ARCH STREQUAL "i386")
    add_asm_files(ntdll_apitest_asm i386/NtContinue.S)
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
add_pch(ntdll_apitest precomp.h SOURCE)

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
