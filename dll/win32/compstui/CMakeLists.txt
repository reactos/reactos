
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(compstui SHARED compstui_main.c)

set_pdef_file(compstui compstui.pdef)
add_importlib_target(${CMAKE_CURRENT_BINARY_DIR}/compstui.def)

set_module_type(compstui win32dll)

target_link_libraries(compstui wine)

add_importlibs(compstui kernel32 ntdll)

add_cab_target(compstui 1)
