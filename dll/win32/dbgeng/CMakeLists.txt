
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(dbgeng.dll dbgeng.spec)

list(APPEND SOURCE
    dbgeng.c
    ${CMAKE_CURRENT_BINARY_DIR}/dbgeng_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dbgeng.def)

add_library(dbgeng MODULE ${SOURCE})
set_module_type(dbgeng win32dll)
target_link_libraries(dbgeng wine)
add_importlibs(dbgeng psapi msvcrt ntdll)
add_delay_importlibs(dbgeng version)
add_cd_file(TARGET dbgeng DESTINATION reactos/system32 FOR all)
