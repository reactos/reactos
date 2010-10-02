
set_unicode()

add_definitions(-D_ADVAPI32_)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_BINARY_DIR}/include/reactos/idl)

list(APPEND SOURCE
    crypt/crypt.c
    crypt/crypt_arc4.c
    crypt/crypt_des.c
    crypt/crypt_lmhash.c
    crypt/crypt_md4.c
    crypt/crypt_md5.c
    crypt/crypt_sha.c
    misc/dllmain.c
    misc/hwprofiles.c
    misc/logon.c
    misc/msi.c
    misc/shutdown.c
    misc/sysfunc.c
    misc/trace.c
    reg/reg.c
    sec/ac.c
    sec/audit.c
    sec/cred.c
    sec/lsa.c
    sec/misc.c
    sec/sec.c
    sec/sid.c
    sec/trustee.c
    service/eventlog.c
    service/rpc.c
    service/scm.c
    service/sctrl.c
    token/privilege.c
    token/token.c
    advapi32_stubs.c
    advapi32.rc)

add_library(advapi32 SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/advapi32_advapi32.h.gch
    ${SOURCE})

set_module_type(advapi32 win32dll)

target_link_libraries(advapi32
    ${CMAKE_CURRENT_SOURCE_DIR}/advapi32.def
    svcctl_client
    lsa_client
    eventlogrpc_client
    wine
    pseh)

add_importlibs(advapi32 rpcrt4 kernel32 ntdll)

add_pch(advapi32 ${CMAKE_CURRENT_SOURCE_DIR}/advapi32.h ${SOURCE})
