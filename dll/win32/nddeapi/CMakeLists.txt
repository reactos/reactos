
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(nddeapi.dll nddeapi.spec)

add_library(nddeapi SHARED nddeapi.c
    ${CMAKE_CURRENT_BINARY_DIR}/nddeapi.def)

set_entrypoint(nddeapi 0)

target_link_libraries(nddeapi wine)

add_importlibs(nddeapi kernel32 ntdll)
add_dependencies(nddeapi psdk buildno_header)
add_cab_target(nddeapi 1)