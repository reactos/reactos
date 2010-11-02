
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

add_library(msimg32 SHARED msimg32_main.c msimg32.def)

if(NOT MSVC)
    set_source_files_properties(msimg32.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(msimg32 win32dll)

target_link_libraries(msimg32 wine)

add_importlibs(msimg32 gdi32 kernel32 ntdll)

add_cab_target(msimg32 1)
