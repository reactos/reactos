
add_library(ufat SHARED ufat.c ufat.rc ufat.def)

if(NOT MSVC)
    set_source_files_properties(ufat.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(ufat win32dll)

target_link_libraries(ufat vfatlib)

add_importlibs(ufat kernel32 ntdll)

add_cab_target(ufat 1)
