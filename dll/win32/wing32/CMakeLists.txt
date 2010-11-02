
set_unicode()

add_library(wing32 SHARED wing32.c wing32.def)

if(NOT MSVC)
    set_source_files_properties(wing32.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(wing32 0)
add_importlibs(wing32 user32 gdi32)
add_dependencies(wing32 psdk)
