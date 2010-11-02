
add_library(untfs SHARED untfs.c untfs.rc untfs.def)

if(NOT MSVC)
    set_source_files_properties(untfs.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(untfs win32dll)

target_link_libraries(untfs ntfslib)

add_importlibs(untfs kernel32 ntdll)

add_cab_target(untfs 1)
