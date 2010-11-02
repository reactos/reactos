
set_unicode()

add_library(avicap32 SHARED avicap32.c avicap32.rc avicap32.def)

if(NOT MSVC)
    set_source_files_properties(avicap32.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(avicap32 win32dll)

target_link_libraries(avicap32 wine)

add_importlibs(avicap32 advapi32 user32 version kernel32 ntdll)

add_cab_target(avicap32 1)
