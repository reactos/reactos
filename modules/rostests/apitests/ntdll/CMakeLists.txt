
list(APPEND SOURCE
    LdrEnumResources.c
    NtAcceptConnectPort.c
    NtAllocateVirtualMemory.c
    NtApphelpCacheControl.c
    NtContinue.c
    NtCreateFile.c
    NtCreateKey.c
    NtCreateThread.c
    NtDeleteKey.c
    NtFreeVirtualMemory.c
    NtLoadUnloadKey.c
    NtMapViewOfSection.c
    NtMutant.c
    NtOpenKey.c
    NtOpenProcessToken.c
    NtOpenThreadToken.c
    NtProtectVirtualMemory.c
    NtQueryInformationProcess.c
    NtQueryKey.c
    NtQuerySystemEnvironmentValue.c
    NtQueryVolumeInformationFile.c
    NtReadFile.c
    NtSaveKey.c
    NtSetValueKey.c
    NtWriteFile.c
    RtlAllocateHeap.c
    RtlBitmap.c
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
    RtlInitializeBitMap.c
    RtlIsNameLegalDOS8Dot3.c
    RtlMemoryStream.c
    RtlNtPathNameToDosPathName.c
    RtlpEnsureBufferSize.c
    RtlReAllocateHeap.c
    RtlUnicodeStringToAnsiString.c
    RtlUpcaseUnicodeStringToCountedOemString.c
    StackOverflow.c
    SystemInfo.c
    Timer.c
    precomp.h)

if(ARCH STREQUAL "i386")
    add_asm_files(ntdll_apitest_asm i386/NtContinue.S)
endif()

add_executable(ntdll_apitest ${SOURCE} ${ntdll_apitest_asm} testlist.c)
target_link_libraries(ntdll_apitest wine uuid ${PSEH_LIB})
set_module_type(ntdll_apitest win32cui)
add_importlibs(ntdll_apitest msvcrt advapi32 kernel32 ntdll)
add_pch(ntdll_apitest precomp.h SOURCE)

if(NOT MSVC)
    set_source_files_properties(RtlGetFullPathName_UstrEx.c PROPERTIES COMPILE_FLAGS "-Wno-format")
endif()

add_rostests_file(TARGET ntdll_apitest)
add_rostests_file(TARGET ntdll_apitest SUBDIR testdata)
# These are empty files
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/ntdll_apitest.exe.local" SUBDIR testdata)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/shell32.dll" SUBDIR testdata)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/test.dll" SUBDIR testdata)
