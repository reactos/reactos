
list(APPEND SOURCE
    globals.c
    registry.c
    rpcsrv.c
    netbios.c
    svchost.c)

list(APPEND PCH_SKIP_SOURCE
    security.cxx)

add_executable(svchost
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    svchost.rc)

target_link_libraries(svchost uuid)
set_module_type(svchost win32cui UNICODE)
add_delay_importlibs(svchost netapi32 ole32)
add_importlibs(svchost advapi32 rpcrt4 kernel32 ntdll)
add_pch(svchost svchost.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET svchost DESTINATION reactos/system32 FOR all)
