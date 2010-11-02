
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(lz32 SHARED version.rc lz32.def)

if(NOT MSVC)
    set_source_files_properties(lz32.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(lz32 0)

target_link_libraries(lz32 wine)

add_importlibs(lz32 kernel32 ntdll)
add_dependencies(lz32 psdk buildno_header)
add_cab_target(lz32 1)