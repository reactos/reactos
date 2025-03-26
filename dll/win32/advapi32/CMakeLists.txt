
spec2def(advapi32.dll advapi32.spec ADD_IMPORTLIB)
add_definitions(-D_ADVAPI32_)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers/ksecdd
    ${REACTOS_SOURCE_DIR}/sdk/lib/cryptlib
    ${CMAKE_CURRENT_BINARY_DIR})

add_rpc_files(client
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/eventlogrpc.idl
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/lsa.idl
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/seclogon.idl
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/svcctl.idl
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/winreg.idl)

list(APPEND SOURCE
    misc/dllmain.c
    misc/efs.c
    misc/evntrace.c
    misc/hwprofiles.c
    misc/logon.c
    misc/msi.c
    misc/shutdown.c
    misc/sysfunc.c
    misc/unicode.c
    reg/hkcr.c
    reg/reg.c
    sec/ac.c
    sec/audit.c
    sec/lsa.c
    sec/misc.c
    sec/safer.c
    sec/sec.c
    sec/trustee.c
    service/eventlog.c
    service/rpc.c
    service/scm.c
    service/sctrl.c
    service/winreg.c
    token/token.c
    wine/cred.c
    wine/crypt.c
    wine/crypt_des.c
    wine/crypt_lmhash.c
    wine/security.c)

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/advapi32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/eventlogrpc_c.c
    ${CMAKE_CURRENT_BINARY_DIR}/lsa_c.c
    ${CMAKE_CURRENT_BINARY_DIR}/seclogon_c.c
    ${CMAKE_CURRENT_BINARY_DIR}/svcctl_c.c
    ${CMAKE_CURRENT_BINARY_DIR}/winreg_c.c)

add_library(advapi32 MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    advapi32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/advapi32.def)

set_module_type(advapi32 win32dll UNICODE ENTRYPOINT DllMain 12)
target_link_libraries(advapi32 cryptlib wine ${PSEH_LIB})
if(DLL_EXPORT_VERSION GREATER_EQUAL 0x600)
    target_link_libraries(advapi32 etwtrace)
endif()
add_delay_importlibs(advapi32 secur32)
add_importlibs(advapi32 advapi32_vista rpcrt4 kernel32 ntdll)
add_pch(advapi32 advapi32.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET advapi32 DESTINATION reactos/system32 FOR all)
