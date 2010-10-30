
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(rsabase.dll rsabase.spec)

add_library(rsabase SHARED version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/rsabase.def)

set_entrypoint(rsabase 0)

target_link_libraries(rsabase wine)

add_importlibs(rsabase rsaenh ntdll)

add_cab_target(rsabase 1)