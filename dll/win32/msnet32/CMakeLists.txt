
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

add_library(msnet32 MODULE msnet_main.c)
spec2def(msnet32 msnet32.dll msnet32.spec)

set_module_type(msnet32 win32dll)
target_link_libraries(msnet32 wine)
add_importlibs(msnet32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msnet32 DESTINATION reactos/system32 FOR all)
