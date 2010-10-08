
include_directories(include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msgina ${CMAKE_CURRENT_SOURCE_DIR}/msgina.spec ${CMAKE_CURRENT_BINARY_DIR}/msgina.def)

add_library(msgina SHARED
    gui.c
    msgina.c
    stubs.c
    tui.c
    msgina.rc)

set_module_type(msgina win32dll)

target_link_libraries(msgina
    ${CMAKE_CURRENT_BINARY_DIR}/msgina.def
    wine)

add_importlibs(msgina advapi32 user32 gdi32 userenv kernel32 ntdll)
add_dependencies(msgina msgina_def)
