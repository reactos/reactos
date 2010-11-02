
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(msisys SHARED msisys.c msisys.def)

if(NOT MSVC)
    set_source_files_properties(msisys.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(msisys win32ocx)

target_link_libraries(msisys
    uuid
    wine)

add_importlibs(msisys kernel32 ntdll)

