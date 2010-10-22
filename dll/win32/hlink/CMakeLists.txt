
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(hlink ${CMAKE_CURRENT_SOURCE_DIR}/hlink.spec ${CMAKE_CURRENT_BINARY_DIR}/hlink.def)

add_library(hlink SHARED
    browse_ctx.c
    extserv.c
    hlink_main.c
    link.c)

set_module_type(hlink win32dll)

target_link_libraries(hlink
    ${CMAKE_CURRENT_BINARY_DIR}/hlink.def
    uuid
    wine)

add_importlibs(hlink
    shell32
    ole32
    advapi32
    urlmon
    kernel32
    ntdll)

add_dependencies(hlink hlink_def)
add_livecd_target(hlink reactos/system32)
