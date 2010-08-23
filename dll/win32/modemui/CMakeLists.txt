
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_definitions(-DUNICODE -D_UNICODE)

spec2def(modemui ${CMAKE_CURRENT_SOURCE_DIR}/modemui.spec ${CMAKE_CURRENT_BINARY_DIR}/modemui.def)

add_library(modemui SHARED modemui.c modemui.rc)

set_target_properties(modemui PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(modemui
    ${CMAKE_CURRENT_BINARY_DIR}/modemui.def
    ${REACTOS_SOURCE_DIR}/dll/win32/kernel32/libkernel32.a)

add_dependencies(modemui modemui_def psdk)
