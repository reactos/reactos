
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

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
