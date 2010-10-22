
spec2def(rasdlg ${CMAKE_CURRENT_SOURCE_DIR}/rasdlg.spec ${CMAKE_CURRENT_BINARY_DIR}/rasdlg.def)

add_library(rasdlg SHARED rasdlg.c rasdlg.rc)

set_module_type(rasdlg win32dll)

target_link_libraries(rasdlg
    ${CMAKE_CURRENT_BINARY_DIR}/rasdlg.def
    wine
    uuid)

add_importlibs(rasdlg kernel32 ntdll)
add_dependencies(rasdlg rasdlg_def)
add_livecd_target(rasdlg reactos/system32)
