
add_definitions(
    -D__WINESRC__
    -D_DLL -D__USE_CRTIMP)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(msrle32 SHARED msrle32.c rsrc.rc msrle32.def)

if(NOT MSVC)
    set_source_files_properties(msrle32.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(msrle32 win32dll)

target_link_libraries(msrle32 wine)

add_importlibs(msrle32 msvcrt winmm user32 kernel32 ntdll)

add_cab_target(msrle32 1)
