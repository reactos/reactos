
list(APPEND SOURCE
    NtAllocateVirtualMemory.c
    NtFreeVirtualMemory.c
    RtlGetFullPathName_U.c
    RtlGetFullPathName_UstrEx.c
    RtlInitializeBitMap.c
    SystemInfo.c
    ZwContinue.c
    testlist.c)

if(ARCH MATCHES i386)
    list(APPEND SOURCE i386/ZwContinue.S)
endif()

add_executable(ntdll_apitest ${SOURCE})
target_link_libraries(ntdll_apitest wine ${PSEH_LIB})
set_module_type(ntdll_apitest win32cui)
add_importlibs(ntdll_apitest msvcrt advapi32 kernel32 ntdll)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #add_target_compile_flags(ntdll_apitest "-Wno-format")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-format")
endif()

add_cd_file(TARGET ntdll_apitest DESTINATION reactos/bin FOR all)
