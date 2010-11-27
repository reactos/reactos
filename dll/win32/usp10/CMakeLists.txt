
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(usp10.dll usp10.spec)

add_library(usp10 SHARED
    bidi.c
    usp10.c
    mirror.c
    shape.c
    shaping.c
    ${CMAKE_CURRENT_BINARY_DIR}/usp10.def)

set_module_type(usp10 win32dll)

target_link_libraries(usp10 wine)

add_importlibs(usp10 gdi32 msvcrt kernel32 ntdll)

add_cab_target(usp10 1)
