
add_definitions(-D__WINESRC__)
spec2def(version ${CMAKE_CURRENT_SOURCE_DIR}/version.spec ${CMAKE_CURRENT_BINARY_DIR}/version.def)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(version SHARED
    info.c
    install.c
    resource.c
    version.rc)

set_entrypoint(version 0)

target_link_libraries(version
    ${CMAKE_CURRENT_BINARY_DIR}/version.def
    wine)

add_importlibs(version lz32 kernel32 ntdll)
add_dependencies(version version_def psdk buildno_header)
add_cab_target(version 1)