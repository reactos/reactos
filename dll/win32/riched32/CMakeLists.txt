
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(riched32.dll riched32.spec)

add_library(riched32 SHARED richedit.c version.rc ${CMAKE_CURRENT_BINARY_DIR}/riched32.def)

set_module_type(riched32 win32dll)

target_link_libraries(riched32 wine)

add_importlibs(riched32 riched20 user32 kernel32 ntdll)

add_cab_target(riched32 1)
