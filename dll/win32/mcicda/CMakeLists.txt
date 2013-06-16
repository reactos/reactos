
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mcicda.dll mcicda.spec)

list(APPEND SOURCE
    mcicda.c
    ${CMAKE_CURRENT_BINARY_DIR}/mcicda.def)

add_library(mcicda SHARED ${SOURCE})

set_module_type(mcicda win32dll)
target_link_libraries(mcicda wine)
add_importlibs(mcicda msvcrt winmm user32 kernel32 ntdll)
add_dependencies(mcicda psdk)
add_cd_file(TARGET mcicda DESTINATION reactos/system32 FOR all)
