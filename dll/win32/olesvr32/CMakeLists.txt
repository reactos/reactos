
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(olesvr32.dll olesvr32.spec)

list(APPEND SOURCE
    olesvr_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/olesvr32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/olesvr32.def)

add_library(olesvr32 MODULE ${SOURCE})
set_module_type(olesvr32 win32dll)
target_link_libraries(olesvr32 wine)
add_importlibs(olesvr32 msvcrt kernel32 ntdll)
add_cd_file(TARGET olesvr32 DESTINATION reactos/system32 FOR all)
