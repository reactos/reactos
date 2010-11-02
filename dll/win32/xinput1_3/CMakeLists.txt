
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(xinput1_3 SHARED xinput1_3_main.c version.rc xinput1_3.def)

if(NOT MSVC)
    set_source_files_properties(xinput1_3.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(xinput1_3 win32dll)

target_link_libraries(xinput1_3 wine)

add_importlibs(xinput1_3 msvcrt kernel32 ntdll)

add_cab_target(xinput1_3 1)
