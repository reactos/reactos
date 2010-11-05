
add_library(winfax SHARED winfax.c winfax.rc winfax.def)

if(NOT MSVC)
    set_source_files_properties(winfax.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(winfax 0)

add_importlibs(winfax kernel32 ntdll)

add_dependencies(winfax psdk)

add_cab_target(winfax 1)