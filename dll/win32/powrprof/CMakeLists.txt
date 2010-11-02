
set_unicode()

add_library(powrprof SHARED powrprof.c powrprof.rc powrprof.def)

if(NOT MSVC)
    set_source_files_properties(powrprof.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(powrprof win32dll)

target_link_libraries(powrprof wine)

add_importlibs(powrprof advapi32 user32 comctl32 kernel32 ntdll)

add_cab_target(powrprof 1)
