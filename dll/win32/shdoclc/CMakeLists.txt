
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()
spec2def(shdoclc.dll shdoclc.spec)

set_rc_compiler()

add_library(shdoclc SHARED rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shdoclc.def)

set_entrypoint(shdoclc 0)

target_link_libraries(shdoclc wine)

add_importlibs(shdoclc kernel32 ntdll)
add_dependencies(shdoclc psdk)

add_cab_target(shdoclc 1)