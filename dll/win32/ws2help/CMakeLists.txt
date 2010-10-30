
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/winsock)

spec2def(ws2help.dll ws2help.spec)

add_library(ws2help SHARED
    apc.c
    context.c
    dllmain.c
    handle.c
    notify.c
    ${CMAKE_CURRENT_BINARY_DIR}/ws2help.def)

set_module_type(ws2help win32dll)

add_importlibs(ws2help advapi32 ws2_32 kernel32 ntdll)

add_cab_target(ws2help 1)
