
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(sxs.dll sxs.spec)

add_library(sxs SHARED sxs.c
    ${CMAKE_CURRENT_BINARY_DIR}/sxs.def)

set_entrypoint(sxs 0)

target_link_libraries(sxs wine)

add_importlibs(sxs kernel32 ntdll)
add_dependencies(sxs psdk)

add_cab_target(sxs 1)