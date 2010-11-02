
set_unicode()

add_library(wsock32 SHARED stubs.c wsock32.rc wsock32.def)

if(NOT MSVC)
    set_source_files_properties(wsock32.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(wsock32 win32dll)

add_importlibs(wsock32 ws2_32 kernel32 ntdll)
add_cab_target(wsock32 1)
