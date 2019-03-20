
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(riched32.dll riched32.spec)

list(APPEND SOURCE
    richedit.c
    ${CMAKE_CURRENT_BINARY_DIR}/riched32.def)

add_library(riched32 MODULE ${SOURCE} version.rc)
set_module_type(riched32 win32dll)
target_link_libraries(riched32 wine)
add_importlibs(riched32 riched20 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET riched32 DESTINATION reactos/system32 FOR all)
