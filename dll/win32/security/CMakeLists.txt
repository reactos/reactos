
set_unicode()

add_definitions(-D__SECURITY__)

spec2def(security ${CMAKE_CURRENT_SOURCE_DIR}/security.spec ${CMAKE_CURRENT_BINARY_DIR}/security.def)

add_library(security SHARED security.rc)

set_entrypoint(security 0)

target_link_libraries(security ${CMAKE_CURRENT_BINARY_DIR}/security.def)

add_importlibs(security advapi32 ntdll)
add_dependencies(security security_def)
