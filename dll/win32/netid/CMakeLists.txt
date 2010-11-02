
add_library(netid SHARED netid.c netid.rc netid.def)

if(NOT MSVC)
    set_source_files_properties(netid.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(netid win32dll)

add_importlibs(netid user32 gdi32 comctl32 advapi32 netapi32 kernel32 ntdll)

add_cab_target(netid 1)
