
spec2def(srclient ${CMAKE_CURRENT_SOURCE_DIR}/srclient.spec ${CMAKE_CURRENT_BINARY_DIR}/srclient.def)

add_library(srclient SHARED srclient_main.c)

set_entrypoint(srclient 0)

target_link_libraries(srclient ${CMAKE_CURRENT_BINARY_DIR}/srclient.def)

add_importlibs(srclient kernel32 ntdll)
add_dependencies(srclient srclient_def psdk)
