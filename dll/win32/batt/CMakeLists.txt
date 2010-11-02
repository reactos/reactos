
set_unicode()

add_library(batt SHARED batt.c batt.rc batt.def)

if(NOT MSVC)
    set_source_files_properties(batt.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(batt win32dll)

add_importlibs(batt kernel32)

add_cab_target(batt 1)
