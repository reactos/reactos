
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(riched32 SHARED richedit.c version.rc riched32.def)

if(NOT MSVC)
    set_source_files_properties(riched32.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(riched32 win32dll)

target_link_libraries(riched32 wine)

add_importlibs(riched32 riched20 user32 msvcrt kernel32 ntdll)

add_cab_target(riched32 1)
