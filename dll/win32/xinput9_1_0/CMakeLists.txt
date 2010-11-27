
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(xinput9_1_0 SHARED xinput9_1_0_main.c version.rc xinput9_1_0.def)

if(NOT MSVC)
    set_source_files_properties(xinput9_1_0.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(xinput9_1_0 win32dll)

add_importlibs(xinput9_1_0 msvcrt kernel32)

add_cab_target(xinput9_1_0 1)
