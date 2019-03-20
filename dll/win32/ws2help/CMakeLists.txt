
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/winsock)
spec2def(ws2help.dll ws2help.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    apc.c
    context.c
    dllmain.c
    handle.c
    notify.c
    precomp.h)

add_library(ws2help MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/ws2help.def)

set_module_type(ws2help win32dll)
add_importlibs(ws2help advapi32 ws2_32 msvcrt kernel32 ntdll)
add_pch(ws2help precomp.h SOURCE)
add_cd_file(TARGET ws2help DESTINATION reactos/system32 FOR all)
