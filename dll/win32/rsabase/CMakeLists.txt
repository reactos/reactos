
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(rsabase SHARED version.rc rsabase.def)

if(NOT MSVC)
    set_source_files_properties(rsabase.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(rsabase 0)

target_link_libraries(rsabase wine)

add_importlibs(rsabase rsaenh ntdll)

add_cab_target(rsabase 1)