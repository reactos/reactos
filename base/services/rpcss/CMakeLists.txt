
set_unicode()

add_executable(rpcss
    epmp.c
    irotp.c
    rpcss_main.c
    service_main.c
    epm.idl
    irot.idl
    rpcss.rc)

target_link_libraries(rpcss rpcss_epm_server rpcss_irot_server pseh)

add_pch(rpcss ${CMAKE_CURRENT_SOURCE_DIR}/rpcss.h ${SOURCE})

set_module_type(rpcss win32cui)

add_importlibs(rpcss advapi32 rpcrt4 msvcrt kernel32 ntdll)
