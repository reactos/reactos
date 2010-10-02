
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(lz32 ${CMAKE_CURRENT_SOURCE_DIR}/lz32.spec ${CMAKE_CURRENT_BINARY_DIR}/lz32.def)

add_library(lz32 SHARED version.rc)

set_entrypoint(lz32 0)

target_link_libraries(lz32
    ${CMAKE_CURRENT_BINARY_DIR}/lz32.def
    wine)

add_importlibs(lz32 kernel32 ntdll)
add_dependencies(lz32 lz32_def psdk buildno_header)
