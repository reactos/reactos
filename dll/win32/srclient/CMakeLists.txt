


spec2def(srclient ${CMAKE_CURRENT_SOURCE_DIR}/srclient.spec ${CMAKE_CURRENT_BINARY_DIR}/srclient.def)

add_library(srclient SHARED srclient_main.c)

set_target_properties(srclient PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(srclient
    ${CMAKE_CURRENT_BINARY_DIR}/srclient.def
    -lkernel32
    -lntdll)

add_dependencies(srclient srclient_def psdk)
