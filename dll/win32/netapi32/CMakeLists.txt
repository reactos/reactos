
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(netapi32 ${CMAKE_CURRENT_SOURCE_DIR}/netapi32.spec ${CMAKE_CURRENT_BINARY_DIR}/netapi32.def)

add_library(netapi32 SHARED
    access.c
    apibuf.c
    browsr.c
    ds.c
    local_group.c
    nbcmdqueue.c
    nbnamecache.c
    nbt.c
    netapi32.c
    netbios.c
    share.c
    wksta.c)

set_module_type(netapi32 win32dll)

target_link_libraries(netapi32
    ${CMAKE_CURRENT_BINARY_DIR}/netapi32.def
    wine)

add_importlibs(netapi32 iphlpapi ws2_32 advapi32 kernel32 ntdll)
add_dependencies(netapi32 netapi32_def)
add_livecd_target(netapi32 reactos/system32)
