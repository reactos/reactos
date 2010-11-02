
add_library(ufatx SHARED ufatx.rc ufatx.def)

if(NOT MSVC)
    set_source_files_properties(ufatx.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(ufatx 0)

target_link_libraries(ufatx vfatxlib)

add_importlibs(ufatx gcc kernel32 ntdll)
add_dependencies(ufatx psdk buildno_header)
add_cab_target(ufatx 1)