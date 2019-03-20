
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(imaadp32.acm imaadp32.acm.spec)

list(APPEND SOURCE
    imaadp32.c
    ${CMAKE_CURRENT_BINARY_DIR}/imaadp32.def)

add_library(imaadp32.acm MODULE ${SOURCE})
set_module_type(imaadp32.acm win32dll)
set_target_properties(imaadp32.acm PROPERTIES SUFFIX "")
target_link_libraries(imaadp32.acm wine)
add_importlibs(imaadp32.acm winmm user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET imaadp32.acm DESTINATION reactos/system32 FOR all)
