
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(rsabase ${CMAKE_CURRENT_SOURCE_DIR}/rsabase.spec ${CMAKE_CURRENT_BINARY_DIR}/rsabase.def)

add_library(rsabase SHARED version.rc)

set_entrypoint(rsabase 0)

target_link_libraries(rsabase
    ${CMAKE_CURRENT_BINARY_DIR}/rsabase.def
    wine)

add_importlibs(rsabase rsaenh ntdll)
add_dependencies(rsabase rsabase_def)
add_livecd_target(rsabase reactos/system32)