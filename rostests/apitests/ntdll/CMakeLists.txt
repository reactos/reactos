
add_definitions(-D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    NtFreeVirtualMemory.c
    RtlInitializeBitMap.c
    SystemInfo.c
    ZwContinue.c
    testlist.c)

if(ARCH MATCHES i386)
    list(APPEND SOURCE i386/ZwContinue.S)
endif()

add_executable(ntdll_apitest ${SOURCE})
target_link_libraries(ntdll_apitest wine)
set_module_type(ntdll_apitest win32cui)
add_importlibs(ntdll_apitest msvcrt advapi32 kernel32 ntdll)
add_cd_file(TARGET ntdll_apitest DESTINATION reactos/bin FOR all)
