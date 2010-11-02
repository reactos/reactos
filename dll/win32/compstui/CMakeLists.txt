
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(compstui SHARED compstui_main.c compstui.def)

if(NOT MSVC)
    set_source_files_properties(compstui.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(compstui win32dll)

target_link_libraries(compstui wine)

add_importlibs(compstui kernel32 ntdll)

add_cab_target(compstui 1)
