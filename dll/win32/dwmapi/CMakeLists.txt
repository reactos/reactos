
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(dwmapi.dll dwmapi.spec)

list(APPEND SOURCE
     dwmapi_main.c)

add_library(dwmapi MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dwmapi.def
    ${CMAKE_CURRENT_BINARY_DIR}/dwmapi_stubs.c)

set_module_type(dwmapi win32dll UNICODE ENTRYPOINT 0)
target_link_libraries(dwmapi uuid wine)
add_importlibs(dwmapi user32 kernel32 ntdll)
add_cd_file(TARGET dwmapi DESTINATION reactos/system32 FOR all)
