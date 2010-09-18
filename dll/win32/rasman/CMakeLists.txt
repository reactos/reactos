


spec2def(rasman ${CMAKE_CURRENT_SOURCE_DIR}/rasman.spec ${CMAKE_CURRENT_BINARY_DIR}/rasman.def)

add_library(rasman SHARED rasman.c rasman.rc)

set_target_properties(rasman PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(rasman
    ${CMAKE_CURRENT_BINARY_DIR}/rasman.def
    wine
    -luuid
    mingw_common)

add_dependencies(rasman rasman_def psdk)
