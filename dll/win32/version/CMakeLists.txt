
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(version SHARED
    info.c
    install.c
    resource.c
    version.rc
    version.def)

if(NOT MSVC)
    set_source_files_properties(version.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(version 0)

target_link_libraries(version wine)

add_importlibs(version lz32 kernel32 ntdll)
add_dependencies(version psdk buildno_header)
add_cab_target(version 1)