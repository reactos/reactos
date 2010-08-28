
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

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
