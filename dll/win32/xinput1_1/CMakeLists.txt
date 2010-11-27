
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(xinput1_1 SHARED xinput1_1_main.c version.rc xinput1_1.def)

if(NOT MSVC)
    set_source_files_properties(xinput1_1.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(xinput1_1 win32dll)

add_importlibs(xinput1_1 msvcrt kernel32)

add_cab_target(xinput1_1 1)
