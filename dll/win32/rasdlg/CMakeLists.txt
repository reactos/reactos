
list(APPEND SOURCE
    rasdlg.c
    rasdlg.rc)

add_library(rasdlg MODULE ${SOURCE})
spec2def(rasdlg rasdlg.dll rasdlg.spec)

set_module_type(rasdlg win32dll)
target_link_libraries(rasdlg wine uuid)
add_importlibs(rasdlg msvcrt kernel32 ntdll)
add_cd_file(TARGET rasdlg DESTINATION reactos/system32 FOR all)
