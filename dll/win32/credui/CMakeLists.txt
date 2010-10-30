
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(credui.dll credui.spec)

add_library(credui SHARED credui_main.c credui.rc ${CMAKE_CURRENT_BINARY_DIR}/credui.def)

set_module_type(credui win32dll)

target_link_libraries(credui wine)

add_importlibs(credui advapi32 user32 comctl32 kernel32 ntdll)

add_cab_target(credui 1)
