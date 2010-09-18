


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
