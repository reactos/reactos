
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(localui ${CMAKE_CURRENT_SOURCE_DIR}/localui.spec ${CMAKE_CURRENT_BINARY_DIR}/localui.def)

add_library(localui SHARED localui.c localui.rc)

set_module_type(localui win32dll)

target_link_libraries(localui
    ${CMAKE_CURRENT_BINARY_DIR}/localui.def
    uuid
    wine)

add_importlibs(localui winspool user32 kernel32 ntdll)
add_dependencies(localui localui_def)
add_cab_target(localui 1)
