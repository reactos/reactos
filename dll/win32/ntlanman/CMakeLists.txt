
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-DUNICODE -D_UNICODE)

spec2def(ntlanman ${CMAKE_CURRENT_SOURCE_DIR}/ntlanman.spec ${CMAKE_CURRENT_BINARY_DIR}/ntlanman.def)

add_library(ntlanman SHARED ntlanman.c ntlanman.rc)

set_target_properties(ntlanman PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(ntlanman
    ${CMAKE_CURRENT_BINARY_DIR}/ntlanman.def
    wine
    mingw_common
    -lnetapi32)

add_dependencies(ntlanman ntlanman_def psdk)
