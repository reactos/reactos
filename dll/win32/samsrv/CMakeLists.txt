
set_unicode()

add_library(samsrv SHARED samsrv.c samsrv.rc samsrv.def)

if(NOT MSVC)
    set_source_files_properties(samsrv.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(samsrv 0)

add_importlibs(samsrv kernel32 ntdll)
add_dependencies(samsrv psdk buildno_header)
