
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
set_wine_module_FIXME(dbgeng) # CORE-5743: No ARRAY_SIZE and CONST_VTABLE
