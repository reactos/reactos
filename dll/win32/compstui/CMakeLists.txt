
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(compstui ${CMAKE_CURRENT_SOURCE_DIR}/compstui.spec ${CMAKE_CURRENT_BINARY_DIR}/compstui.def)

add_library(compstui SHARED compstui_main.c)

set_module_type(compstui win32dll)

target_link_libraries(compstui
    ${CMAKE_CURRENT_BINARY_DIR}/compstui.def
    wine)

add_importlibs(compstui kernel32 ntdll)
add_dependencies(compstui compstui_def)
