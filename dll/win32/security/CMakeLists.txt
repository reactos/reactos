set_unicode()
add_definitions(-D__SECURITY__)

set_rc_compiler()
spec2def(security.dll security.spec)

list(APPEND SOURCE
    security.rc
    ${CMAKE_CURRENT_BINARY_DIR}/security_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/security.def)

add_library(security SHARED ${SOURCE})
set_entrypoint(security 0)

add_importlibs(security advapi32 ntdll kernel32)

add_cab_target(security 1)
