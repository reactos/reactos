
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/winsock)

spec2def(ws2help ${CMAKE_CURRENT_SOURCE_DIR}/ws2help.spec ${CMAKE_CURRENT_BINARY_DIR}/ws2help.def)

add_library(ws2help SHARED
    apc.c
    context.c
    dllmain.c
    handle.c
    notify.c)

set_module_type(ws2help win32dll)

target_link_libraries(ws2help ${CMAKE_CURRENT_BINARY_DIR}/ws2help.def)
add_importlibs(ws2help advapi32 ws2_32 kernel32 ntdll)
add_dependencies(ws2help ws2help_def)
add_cab_target(ws2help 1)
