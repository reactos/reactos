
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

spec2def(rasdlg ${CMAKE_CURRENT_SOURCE_DIR}/rasdlg.spec ${CMAKE_CURRENT_BINARY_DIR}/rasdlg.def)

add_library(rasdlg SHARED rasdlg.c rasdlg.rc)

set_target_properties(rasdlg PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(rasdlg
    ${CMAKE_CURRENT_BINARY_DIR}/rasdlg.def
    wine
    -luuid
    mingw_common)

add_dependencies(rasdlg rasdlg_def psdk)
