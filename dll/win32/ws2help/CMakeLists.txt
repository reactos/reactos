
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/winsock)

add_library(ws2help SHARED
    apc.c
    context.c
    dllmain.c
    handle.c
    notify.c
    ws2help.def)

if(NOT MSVC)
    set_source_files_properties(ws2help.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(ws2help win32dll)

add_importlibs(ws2help advapi32 ws2_32 kernel32 ntdll)

add_cab_target(ws2help 1)
