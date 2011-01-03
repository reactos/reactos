
#add_definitions(-D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    RtlInitializeBitMap.c
    ZwContinue.c
    testlist.c)

if(ARCH MATCHES i386)
list(APPEND SOURCE i386/ZwContinue.asm)
endif()

add_executable(ntdll_apitest ${SOURCE})
target_link_libraries(ntdll_apitest wine
 #${PSEH_LIB}
 )
set_module_type(ntdll_apitest win32cui)
add_importlibs(ntdll_apitest ntdll)
add_cab_target(ntdll_apitest 7)
