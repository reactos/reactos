
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(xinput1_2 SHARED xinput1_2_main.c version.rc xinput1_2.def)

if(NOT MSVC)
    set_source_files_properties(xinput1_2.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

add_importlibs(xinput1_2 kernel32)

set_module_type(xinput1_2 win32dll)

add_cab_target(xinput1_2 1)
