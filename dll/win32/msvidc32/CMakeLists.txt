
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(
    -D__WINESRC__
    -D_WINE)

add_library(msvidc32 SHARED msvideo1.c rsrc.rc msvidc32.def)

if(NOT MSVC)
    set_source_files_properties(msvidc32.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(msvidc32 win32dll)

target_link_libraries(msvidc32 wine)

add_importlibs(msvidc32 user32 kernel32 ntdll)

add_cab_target(msvidc32 1)
