


spec2def(rasdlg ${CMAKE_CURRENT_SOURCE_DIR}/rasdlg.spec ${CMAKE_CURRENT_BINARY_DIR}/rasdlg.def)

add_library(rasdlg SHARED rasdlg.c rasdlg.rc)

set_target_properties(rasdlg PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(rasdlg
    ${CMAKE_CURRENT_BINARY_DIR}/rasdlg.def
    wine
    -luuid
    mingw_common)

add_dependencies(rasdlg rasdlg_def psdk)
