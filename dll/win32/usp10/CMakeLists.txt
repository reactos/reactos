
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(usp10 ${CMAKE_CURRENT_SOURCE_DIR}/usp10.spec ${CMAKE_CURRENT_BINARY_DIR}/usp10.def)

add_library(usp10 SHARED
    bidi.c
    usp10.c
    mirror.c
    shape.c
    shaping.c)

set_module_type(usp10 win32dll)

target_link_libraries(usp10
    ${CMAKE_CURRENT_BINARY_DIR}/usp10.def
    wine)

add_importlibs(usp10 gdi32 kernel32 ntdll)
add_dependencies(usp10 usp10_def)
