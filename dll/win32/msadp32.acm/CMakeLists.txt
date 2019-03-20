
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msadp32.acm msadp32.acm.spec)

list(APPEND SOURCE
    msadp32.c
    ${CMAKE_CURRENT_BINARY_DIR}/msadp32.def)

add_library(msadp32.acm MODULE ${SOURCE})
set_module_type(msadp32.acm win32dll)
set_target_properties(msadp32.acm PROPERTIES SUFFIX "")
target_link_libraries(msadp32.acm wine)
add_importlibs(msadp32.acm winmm user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msadp32.acm DESTINATION reactos/system32 FOR all)
