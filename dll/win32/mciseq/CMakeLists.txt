
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mciseq ${CMAKE_CURRENT_SOURCE_DIR}/mciseq.spec ${CMAKE_CURRENT_BINARY_DIR}/mciseq.def)

add_library(mciseq SHARED mcimidi.c)

set_entrypoint(mciseq 0)

target_link_libraries(mciseq
    ${CMAKE_CURRENT_BINARY_DIR}/mciseq.def
    wine)

add_importlibs(mciseq winmm user32 kernel32 ntdll)
add_dependencies(mciseq mciseq_def psdk buildno_header)
add_livecd_target(mciseq reactos/system32)