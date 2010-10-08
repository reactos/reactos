
set_unicode()

add_definitions(-DLE)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mswsock ${CMAKE_CURRENT_SOURCE_DIR}/mswsock.spec ${CMAKE_CURRENT_BINARY_DIR}/mswsock.def)

add_library(mswsock SHARED
    extensions.c
    stubs.c
    mswsock.rc)

set_module_type(mswsock win32dll)

target_link_libraries(mswsock ${CMAKE_CURRENT_BINARY_DIR}/mswsock.def)

add_importlibs(mswsock ws2_32 kernel32)
add_dependencies(mswsock mswsock_def)
