
spec2def(advapi32.dll advapi32.spec ADD_IMPORTLIB)

add_definitions(-D_ADVAPI32_)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/idl
    ${CMAKE_CURRENT_BINARY_DIR})

add_rpc_files(client
    ${REACTOS_SOURCE_DIR}/include/reactos/idl/eventlogrpc.idl
    ${REACTOS_SOURCE_DIR}/include/reactos/idl/lsa.idl
    ${REACTOS_SOURCE_DIR}/include/reactos/idl/svcctl.idl)
    

list(APPEND SOURCE
    crypt/crypt.c
    crypt/crypt_arc4.c
    crypt/crypt_des.c
    crypt/crypt_lmhash.c
    crypt/crypt_md4.c
    crypt/crypt_md5.c
    crypt/crypt_sha.c
    misc/dllmain.c
    misc/efs.c
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
    advapi32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/advapi32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/advapi32.def
    ${CMAKE_CURRENT_BINARY_DIR}/eventlogrpc_c.c
    ${CMAKE_CURRENT_BINARY_DIR}/lsa_c.c
    ${CMAKE_CURRENT_BINARY_DIR}/svcctl_c.c)

add_library(advapi32 SHARED ${SOURCE})
set_module_type(advapi32 win32dll UNICODE)
target_link_libraries(advapi32 wine ${PSEH_LIB})
add_delay_importlibs(advapi32 rpcrt4)
add_importlibs(advapi32 msvcrt kernel32 ntdll)

add_pch(advapi32 advapi32.h
    ${CMAKE_CURRENT_BINARY_DIR}/lsa_c.h
    ${CMAKE_CURRENT_BINARY_DIR}/eventlogrpc_c.h)

add_cd_file(TARGET advapi32 DESTINATION reactos/system32 FOR all)

