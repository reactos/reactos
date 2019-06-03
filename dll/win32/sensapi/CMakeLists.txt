
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    sensapi.c)

add_library(sensapi MODULE ${SOURCE})
spec2def(sensapi sensapi.dll sensapi.spec ADD_IMPORTLIB)

set_module_type(sensapi win32dll)
target_link_libraries(sensapi wine)
add_importlibs(sensapi msvcrt kernel32 ntdll)
add_cd_file(TARGET sensapi DESTINATION reactos/system32 FOR all)
