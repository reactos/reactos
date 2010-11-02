
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

add_library(smdll SHARED
    dllmain.c
    query.c
    smdll.rc
    smdll.def)

if(NOT MSVC)
    set_source_files_properties(smdll.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

target_link_libraries(smdll smlib)

add_importlibs(smdll ntdll)
add_dependencies(smdll psdk buildno_header)
add_cab_target(smdll 1)