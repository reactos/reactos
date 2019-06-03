
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    wtsapi32.c)

add_library(wtsapi32 MODULE ${SOURCE})
spec2def(wtsapi32 wtsapi32.dll wtsapi32.spec ADD_IMPORTLIB)

set_module_type(wtsapi32 win32dll)
target_link_libraries(wtsapi32 wine)
add_importlibs(wtsapi32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wtsapi32 DESTINATION reactos/system32 FOR all)
