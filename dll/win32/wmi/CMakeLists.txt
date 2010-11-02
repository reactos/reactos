
add_library(wmi SHARED wmi.rc wmi.def)

if(NOT MSVC)
    set_source_files_properties(wmi.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(wmi 0)

add_importlibs(wmi advapi32)
add_dependencies(wmi psdk buildno_header)
