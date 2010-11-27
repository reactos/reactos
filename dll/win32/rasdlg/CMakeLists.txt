
spec2def(rasdlg.dll rasdlg.spec)

add_library(rasdlg SHARED rasdlg.c rasdlg.rc ${CMAKE_CURRENT_BINARY_DIR}/rasdlg.def)

set_module_type(rasdlg win32dll)

target_link_libraries(rasdlg
    wine
    uuid)

add_importlibs(rasdlg msvcrt kernel32 ntdll)

add_cab_target(rasdlg 1)
