
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-DLE)

spec2def(mswsock ${CMAKE_CURRENT_SOURCE_DIR}/mswsock.spec ${CMAKE_CURRENT_BINARY_DIR}/mswsock.def)

add_library(mswsock SHARED
    extensions.c
    stubs.c
    mswsock.rc)

set_target_properties(mswsock PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(mswsock
    ${CMAKE_CURRENT_BINARY_DIR}/mswsock.def
    -lws2_32
    -lkernel32)

add_dependencies(mswsock mswsock_def psdk)
