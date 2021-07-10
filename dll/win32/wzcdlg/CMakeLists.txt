
spec2def(wzcdlg.dll wzcdlg.spec)

list(APPEND SOURCE
    wzcdlg.c
    wzcdlg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wzcdlg_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wzcdlg.def)

add_library(wzcdlg MODULE ${SOURCE})
set_module_type(wzcdlg win32dll)
add_importlibs(wzcdlg msvcrt kernel32 ntdll)
add_cd_file(TARGET wzcdlg DESTINATION reactos/system32 FOR all)
