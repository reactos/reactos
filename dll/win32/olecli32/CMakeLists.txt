
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(olecli32 ${CMAKE_CURRENT_SOURCE_DIR}/olecli32.spec ${CMAKE_CURRENT_BINARY_DIR}/olecli32.def)

add_library(olecli32 SHARED olecli_main.c)

set_entrypoint(olecli32 0)

target_link_libraries(olecli32
    ${CMAKE_CURRENT_BINARY_DIR}/olecli32.def
    wine)

add_importlibs(olecli32 ole32 gdi32 kernel32 ntdll)
add_dependencies(olecli32 olecli32_def psdk buildno_header)
add_livecd_target(olecli32 reactos/system32)