
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(olecli32.dll olecli32.spec)

add_library(olecli32 SHARED olecli_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/olecli32.def)

set_entrypoint(olecli32 0)

target_link_libraries(olecli32 wine)

add_importlibs(olecli32 ole32 gdi32 kernel32 ntdll)
add_dependencies(olecli32 psdk buildno_header)
add_cab_target(olecli32 1)