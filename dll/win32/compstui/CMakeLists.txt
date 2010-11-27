
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(compstui SHARED compstui_main.c ${CMAKE_CURRENT_BINARY_DIR}/compstui.def)

set_module_type(compstui win32dll)

set_export_spec(compstui compstui.spec)

target_link_libraries(compstui wine)

add_importlibs(compstui msvcrt kernel32 ntdll)

add_cab_target(compstui 1)
add_importlib_target(compstui.spec)

