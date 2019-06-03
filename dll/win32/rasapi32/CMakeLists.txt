
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    rasapi.c)

add_library(rasapi32 MODULE ${SOURCE})
spec2def(rasapi32 rasapi32.dll rasapi32.spec ADD_IMPORTLIB)

set_module_type(rasapi32 win32dll)
target_link_libraries(rasapi32 wine)
add_importlibs(rasapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET rasapi32 DESTINATION reactos/system32 FOR all)
