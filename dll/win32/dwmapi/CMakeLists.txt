
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dwmapi.dll dwmapi.spec)

list(APPEND SOURCE
    dwmapi_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/dwmapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dwmapi.def)

add_library(dwmapi SHARED ${SOURCE} version.rc)
set_module_type(dwmapi win32dll)
target_link_libraries(dwmapi wine)
add_importlibs(dwmapi msvcrt kernel32 ntdll)
add_cd_file(TARGET dwmapi DESTINATION reactos/system32 FOR all)
