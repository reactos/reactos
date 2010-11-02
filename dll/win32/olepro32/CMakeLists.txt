
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(olepro32 SHARED olepro32stubs.c version.rc olepro32.def)

if(NOT MSVC)
    set_source_files_properties(olepro32.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(olepro32 0)

target_link_libraries(olepro32 wine)

add_importlibs(olepro32
    oleaut32
    kernel32
    ntdll)

add_cab_target(olepro32 1)