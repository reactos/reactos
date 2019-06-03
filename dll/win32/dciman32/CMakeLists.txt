
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    dciman_main.c)

add_library(dciman32 MODULE ${SOURCE})
spec2def(dciman32 dciman32.dll dciman32.spec)

set_module_type(dciman32 win32dll)
target_link_libraries(dciman32 wine)
add_importlibs(dciman32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dciman32 DESTINATION reactos/system32 FOR all)
