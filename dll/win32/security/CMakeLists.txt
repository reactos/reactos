set_unicode()
add_definitions(-D__SECURITY__)

set_rc_compiler()
spec2def(security.dll security.spec)

list(APPEND SOURCE
    security.rc
    ${CMAKE_CURRENT_BINARY_DIR}/security.def)

add_library(security SHARED ${SOURCE})
set_entrypoint(security 0)

add_importlibs(security secur32 advapi32 ntdll kernel32)

add_cd_file(TARGET security DESTINATION reactos/system32 FOR all)
