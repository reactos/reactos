
set_unicode()

add_definitions(-DLE)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mswsock.dll mswsock.spec)

add_library(mswsock SHARED
    extensions.c
    stubs.c
    mswsock.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mswsock.def)

set_module_type(mswsock win32dll)

add_importlibs(mswsock ws2_32 kernel32)

add_cab_target(mswsock 1)
