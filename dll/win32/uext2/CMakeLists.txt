
add_library(uext2 SHARED uext2.c uext2.rc uext2.def)

if(NOT MSVC)
    set_source_files_properties(uext2.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(uext2 win32dll)

target_link_libraries(uext2 ext2lib)

add_importlibs(uext2 msvcrt kernel32 ntdll)

add_cab_target(uext2 1)
