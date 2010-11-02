
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(localui SHARED localui.c localui.rc localui.def)

if(NOT MSVC)
    set_source_files_properties(localui.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(localui win32dll)

target_link_libraries(localui
    uuid
    wine)

add_importlibs(localui winspool user32 kernel32 ntdll)

add_cab_target(localui 1)
