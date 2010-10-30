
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(rasapi32.dll rasapi32.spec)

add_library(rasapi32 SHARED rasapi.c
    ${CMAKE_CURRENT_BINARY_DIR}/rasapi32.def)

set_entrypoint(rasapi32 0)

target_link_libraries(rasapi32 wine)

add_importlibs(rasapi32 kernel32 ntdll)
add_dependencies(rasapi32 psdk buildno_header)
add_cab_target(rasapi32 1)