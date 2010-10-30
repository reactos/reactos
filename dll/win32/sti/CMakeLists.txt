
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -DENTRY_PREFIX=STI_
    -DPROXY_DELEGATION
    -DREGISTER_PROXY_DLL)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(sti ${CMAKE_CURRENT_SOURCE_DIR}/sti.spec ${CMAKE_CURRENT_BINARY_DIR}/sti.def)

add_library(sti SHARED
    regsvr.c
    sti.c
    sti_main.c
    sti_wia.idl)

set_entrypoint(sti 0)

target_link_libraries(sti
    ${CMAKE_CURRENT_BINARY_DIR}/sti.def
    sti_proxy
    wine
    uuid
    pseh)

add_importlibs(sti
    ole32
    oleaut32
    rpcrt4
    advapi32
    kernel32
    ntdll)

add_dependencies(sti sti_def)

rpcproxy(sti sti_wia.idl)
add_cab_target(sti 1)
