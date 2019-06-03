
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    httpapi_main.c)

add_library(httpapi MODULE ${SOURCE})
spec2def(httpapi httpapi.dll httpapi.spec)

set_module_type(httpapi win32dll)
target_link_libraries(httpapi wine)
add_importlibs(httpapi msvcrt kernel32 ntdll)
add_cd_file(TARGET httpapi DESTINATION reactos/system32 FOR all)
