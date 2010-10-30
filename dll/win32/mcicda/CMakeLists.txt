
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mcicda.dll mcicda.spec)

add_library(mcicda SHARED mcicda.c
    ${CMAKE_CURRENT_BINARY_DIR}/mcicda.def)

set_entrypoint(mcicda 0)

target_link_libraries(mcicda wine)

add_importlibs(mcicda winmm user32 kernel32 ntdll)
add_dependencies(mcicda psdk buildno_header)
add_cab_target(mcicda 1)