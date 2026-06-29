
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/winsock)
spec2def(ws2help.dll ws2help.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    apc.c
    context.c
    dllmain.c
    handle.c
    notify.c
    precomp.h)

add_library(ws2help_static ${SOURCE})
add_dependencies(ws2help_static xdk)

add_library(ws2help MODULE
    ws2help.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ws2help.def)

set_module_type(ws2help win32dll)
if(DLL_EXPORT_VERSION LESS 0x600)
    target_link_libraries(ws2help ws2help_static)
    set_entrypoint(ws2help Ws2HelpDllMain 12)
endif()
add_importlibs(ws2help advapi32 ws2_32 msvcrt kernel32 ntdll)
add_pch(ws2help precomp.h SOURCE)
add_cd_file(TARGET ws2help DESTINATION reactos/system32 FOR all)
