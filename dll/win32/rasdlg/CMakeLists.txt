
spec2def(rasdlg.dll rasdlg.spec)

list(APPEND SOURCE
    rasdlg.c
    rasdlg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/rasdlg_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/rasdlg.def)

add_library(rasdlg MODULE ${SOURCE})
set_module_type(rasdlg win32dll)
target_link_libraries(rasdlg wine uuid)
add_importlibs(rasdlg msvcrt kernel32 ntdll)
add_cd_file(TARGET rasdlg DESTINATION reactos/system32 FOR all)
