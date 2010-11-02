
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(mciqtz32 SHARED mciqtz.c version.rc mciqtz32.def)

if(NOT MSVC)
    set_source_files_properties(mciqtz32.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(mciqtz32 win32dll)

target_link_libraries(mciqtz32
    wine
    strmiids)

add_importlibs(mciqtz32 winmm oleaut32 ole32 user32 gdi32 kernel32 ntdll)

add_cab_target(mciqtz32 1)
