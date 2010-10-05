
add_executable(svchost svchost.c svchost.rc)

set_module_type(svchost win32cui)
add_importlibs(svchost advapi32 msvcrt)
