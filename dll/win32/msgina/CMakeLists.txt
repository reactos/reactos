


include_directories(include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msgina ${CMAKE_CURRENT_SOURCE_DIR}/msgina.spec ${CMAKE_CURRENT_BINARY_DIR}/msgina.def)

add_library(msgina SHARED
    gui.c
    msgina.c
    stubs.c
    tui.c
    msgina.rc)

set_target_properties(msgina PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(msgina
    ${CMAKE_CURRENT_BINARY_DIR}/msgina.def
    wine
    mingw_common
    -ladvapi32
    -luser32
    -lgdi32
    -luserenv)

add_dependencies(msgina msgina_def psdk)
