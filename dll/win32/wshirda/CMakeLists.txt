
set_unicode()

add_library(wshirda SHARED wshirda.c wshirda.rc wshirda.def)

if(NOT MSVC)
    set_source_files_properties(wshirda.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(wshirda win32dll)

add_importlibs(wshirda ws2_32 kernel32 ntdll)

add_cab_target(wshirda 1)
