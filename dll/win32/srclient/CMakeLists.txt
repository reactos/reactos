
spec2def(srclient.dll srclient.spec)

list(APPEND SOURCE
    srclient_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/srclient_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/srclient.def)

add_library(srclient SHARED ${SOURCE})
set_entrypoint(srclient 0)

add_importlibs(srclient kernel32 ntdll)
add_dependencies(srclient psdk)

add_cd_file(TARGET srclient DESTINATION reactos/system32 FOR all)
