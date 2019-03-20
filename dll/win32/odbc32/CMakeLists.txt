
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(odbc32.dll odbc32.spec)

list(APPEND SOURCE
    proxyodbc.c
    ${CMAKE_CURRENT_BINARY_DIR}/odbc32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/odbc32.def)

add_library(odbc32 MODULE ${SOURCE})
set_module_type(odbc32 win32dll)
target_link_libraries(odbc32 wine)
add_importlibs(odbc32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET odbc32 DESTINATION reactos/system32 FOR all)
