
spec2def(slbcsp.dll slbcsp.spec)

list(APPEND SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/slbcsp_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/slbcsp.def)

add_library(slbcsp MODULE ${SOURCE})
set_module_type(slbcsp win32dll)
target_link_libraries(slbcsp wine wine_dll_register wine_dll_canunload)
add_importlibs(slbcsp msvcrt kernel32 ntdll)
add_cd_file(TARGET slbcsp DESTINATION reactos/system32 FOR all)
set_wine_module(slbcsp)
