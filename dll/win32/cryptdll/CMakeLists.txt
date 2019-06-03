
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    cryptdll.c
    stubs.c)

add_library(cryptdll MODULE ${SOURCE})
spec2def(cryptdll cryptdll.dll cryptdll.spec)

set_module_type(cryptdll win32dll)
target_link_libraries(cryptdll wine)
add_importlibs(cryptdll advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET cryptdll DESTINATION reactos/system32 FOR all)
