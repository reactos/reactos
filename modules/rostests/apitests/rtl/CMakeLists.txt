
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
    RtlUpcaseUnicodeStringToCountedOemString.c)

add_library(rtl_apitest_lib
    ${RTL_SOURCE}
    testlist.c)

if(NOT MSVC)
    set_source_files_properties(RtlGetFullPathName_UstrEx.c PROPERTIES COMPILE_FLAGS "-Wno-format")

    # Avoid "universal character names are only valid in C++ and C99" error.
    set_property(TARGET rtl_apitest_lib PROPERTY C_STANDARD 99)
endif()

# RTL tests with static linkage
add_executable(rtl_apitest ${RTL_SOURCE} testlist.c ../rtl/libsupp.c ../rtl/rtl_stubs.c)
add_target_compile_flags(rtl_apitest "-D_RTL_TEST -D_NTSYSTEM_")
target_link_libraries(rtl_apitest wine uuid ${PSEH_LIB} rtl)
set_module_type(rtl_apitest win32cui)
add_importlibs(rtl_apitest msvcrt advapi32 kernel32 ntdll)

add_rostests_file(TARGET rtl_apitest)
