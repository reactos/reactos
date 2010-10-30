
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(hlink.dll hlink.spec)

add_library(hlink SHARED
    browse_ctx.c
    extserv.c
    hlink_main.c
    link.c
    ${CMAKE_CURRENT_BINARY_DIR}/hlink.def)

set_module_type(hlink win32dll)

target_link_libraries(hlink
    uuid
    wine)

add_importlibs(hlink
    shell32
    ole32
    advapi32
    urlmon
    kernel32
    ntdll)


add_cab_target(hlink 1)
