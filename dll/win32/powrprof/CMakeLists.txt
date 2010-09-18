


add_definitions(-DUNICODE -D_UNICODE)

spec2def(powrprof ${CMAKE_CURRENT_SOURCE_DIR}/powrprof.spec ${CMAKE_CURRENT_BINARY_DIR}/powrprof.def)

add_library(powrprof SHARED powrprof.c powrprof.rc)

set_target_properties(powrprof PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(powrprof
    ${CMAKE_CURRENT_BINARY_DIR}/powrprof.def
    wine
    -ladvapi32
    -luser32
    -lcomctl32
    mingw_common)

add_dependencies(powrprof powrprof_def psdk)
