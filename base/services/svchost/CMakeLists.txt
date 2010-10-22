
add_executable(svchost svchost.c svchost.rc)

set_module_type(svchost win32cui)
add_importlibs(svchost advapi32 msvcrt kernel32 ntdll)
add_livecd_target(svchost reactos/system32)