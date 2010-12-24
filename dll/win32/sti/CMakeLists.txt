
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -DENTRY_PREFIX=STI_
    -DPROXY_DELEGATION
    -DREGISTER_PROXY_DLL)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(sti.dll sti.spec)

list(APPEND SOURCE
    regsvr.c
    sti.c
    sti_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/sti.def)

add_library(sti SHARED ${SOURCE})

set_entrypoint(sti 0)

target_link_libraries(sti
    sti_proxy
    wine
    uuid
    ${PSEH_LIB})

add_importlibs(sti
    ole32
    oleaut32
    rpcrt4
    advapi32
    kernel32
    ntdll)

rpcproxy(sti sti_wia.idl)
add_cab_target(sti 1)
