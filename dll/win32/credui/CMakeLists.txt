
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(credui ${CMAKE_CURRENT_SOURCE_DIR}/credui.spec ${CMAKE_CURRENT_BINARY_DIR}/credui.def)

add_library(credui SHARED credui_main.c credui.rc)

set_module_type(credui win32dll)

target_link_libraries(credui
    ${CMAKE_CURRENT_BINARY_DIR}/credui.def
    wine)

add_importlibs(credui advapi32 user32 comctl32 kernel32 ntdll)
add_dependencies(credui credui_def)
