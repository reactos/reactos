
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    resutils.c)

add_library(resutils MODULE ${SOURCE})
spec2def(resutils resutils.dll resutils.spec)

set_module_type(resutils win32dll)
target_link_libraries(resutils wine)
add_importlibs(resutils msvcrt kernel32 ntdll)
add_cd_file(TARGET resutils DESTINATION reactos/system32 FOR all)
