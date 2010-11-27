
set_unicode()

add_definitions(-DLE)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mswsock.dll mswsock.spec)

add_library(mswsock SHARED
    extensions.c
    stubs.c
    mswsock.rc
    mswsock.def)

if(NOT MSVC)
    set_source_files_properties(mswsock.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(mswsock win32dll)

add_importlibs(mswsock ws2_32 msvcrt kernel32)

add_cab_target(mswsock 1)
