
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(localui SHARED localui.c localui.rc)

set_pdef_file(localui localui.pdef)
add_importlib_target(${CMAKE_CURRENT_BINARY_DIR}/localui.def)

set_module_type(localui win32dll)

target_link_libraries(localui
    uuid
    wine)

add_importlibs(localui winspool user32 kernel32 ntdll)

add_cab_target(localui 1)
