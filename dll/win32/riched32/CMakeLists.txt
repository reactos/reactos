
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(riched32 ${CMAKE_CURRENT_SOURCE_DIR}/riched32.spec ${CMAKE_CURRENT_BINARY_DIR}/riched32.def)

add_library(riched32 SHARED richedit.c version.rc)

set_module_type(riched32 win32dll)

target_link_libraries(riched32
    ${CMAKE_CURRENT_BINARY_DIR}/riched32.def
    wine)

add_importlibs(riched32 riched20 user32 kernel32 ntdll)
add_dependencies(riched32 riched32_def)
