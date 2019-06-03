
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    nddeapi.c)

add_library(nddeapi MODULE ${SOURCE})
spec2def(nddeapi nddeapi.dll nddeapi.spec)

set_module_type(nddeapi win32dll)
target_link_libraries(nddeapi wine)
add_importlibs(nddeapi msvcrt kernel32 ntdll)
add_cd_file(TARGET nddeapi DESTINATION reactos/system32 FOR all)
