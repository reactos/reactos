


add_definitions(-DUNICODE -D_UNICODE)

spec2def(modemui ${CMAKE_CURRENT_SOURCE_DIR}/modemui.spec ${CMAKE_CURRENT_BINARY_DIR}/modemui.def)

add_library(modemui SHARED modemui.c modemui.rc)

set_target_properties(modemui PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(modemui
    ${CMAKE_CURRENT_BINARY_DIR}/modemui.def
    -lkernel32)

add_dependencies(modemui modemui_def psdk)
