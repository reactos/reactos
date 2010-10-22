
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

add_executable(lsass lsass.c lsass.rc)

set_module_type(lsass win32gui)

add_importlibs(lsass advapi32 lsasrv msvcrt kernel32 ntdll)
add_livecd_target(lsass reactos/system32)