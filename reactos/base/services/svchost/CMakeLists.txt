
add_executable(svchost globals.c registry.c security.cxx rpcsrv.c netbios.c svchost.c svchost.rc)

target_link_libraries(svchost uuid)
set_module_type(svchost win32cui UNICODE)
add_importlibs(svchost advapi32 netapi32 rpcrt4 ole32 kernel32 ntdll)
add_cd_file(TARGET svchost DESTINATION reactos/system32 FOR all)
