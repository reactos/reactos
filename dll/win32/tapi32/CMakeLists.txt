
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(tapi32 ${CMAKE_CURRENT_SOURCE_DIR}/tapi32.spec ${CMAKE_CURRENT_BINARY_DIR}/tapi32.def)

add_library(tapi32 SHARED
    assisted.c
    internal.c
    line.c
    phone.c)

set_entrypoint(tapi32 0)

target_link_libraries(tapi32
    ${CMAKE_CURRENT_BINARY_DIR}/tapi32.def
    wine)

add_importlibs(tapi32 advapi32 kernel32 ntdll)
add_dependencies(tapi32 tapi32_def psdk buildno_header)
