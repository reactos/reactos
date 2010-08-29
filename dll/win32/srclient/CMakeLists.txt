
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

spec2def(srclient ${CMAKE_CURRENT_SOURCE_DIR}/srclient.spec ${CMAKE_CURRENT_BINARY_DIR}/srclient.def)

add_library(srclient SHARED srclient_main.c)

set_target_properties(srclient PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(srclient
    ${CMAKE_CURRENT_BINARY_DIR}/srclient.def
    -lkernel32
    -lntdll)

add_dependencies(srclient srclient_def psdk)
