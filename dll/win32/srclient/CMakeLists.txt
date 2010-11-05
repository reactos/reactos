
spec2def(srclient.dll srclient.spec)

add_library(srclient SHARED srclient_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/srclient.def)

set_entrypoint(srclient 0)

add_importlibs(srclient kernel32 ntdll)
add_dependencies(srclient psdk)

add_cab_target(srclient 1)