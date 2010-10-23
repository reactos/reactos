
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mcicda ${CMAKE_CURRENT_SOURCE_DIR}/mcicda.spec ${CMAKE_CURRENT_BINARY_DIR}/mcicda.def)

add_library(mcicda SHARED mcicda.c)

set_entrypoint(mcicda 0)

target_link_libraries(mcicda
    ${CMAKE_CURRENT_BINARY_DIR}/mcicda.def
    wine)

add_importlibs(mcicda winmm user32 kernel32 ntdll)
add_dependencies(mcicda mcicda_def psdk buildno_header)
add_livecd_target(mcicda reactos/system32)