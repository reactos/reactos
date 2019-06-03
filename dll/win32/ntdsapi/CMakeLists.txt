
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    ntdsapi.c)

add_library(ntdsapi MODULE ${SOURCE})
spec2def(ntdsapi ntdsapi.dll ntdsapi.spec ADD_IMPORTLIB)

set_module_type(ntdsapi win32dll)
target_link_libraries(ntdsapi wine)
add_importlibs(ntdsapi user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ntdsapi DESTINATION reactos/system32 FOR all)
