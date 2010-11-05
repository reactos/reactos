
set_unicode()

add_definitions(-D__SECURITY__)

spec2def(security.dll security.spec)

add_library(security SHARED security.rc
    ${CMAKE_CURRENT_BINARY_DIR}/security.def)

set_entrypoint(security 0)

add_importlibs(security advapi32 ntdll)

add_cab_target(security 1)