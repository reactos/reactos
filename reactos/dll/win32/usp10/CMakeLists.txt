
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(usp10.dll usp10.spec ADD_IMPORTLIB GENERATE_STUBS)

list(APPEND SOURCE
    bidi.c
    usp10.c
    mirror.c
    shape.c
    shaping.c
    ${CMAKE_CURRENT_BINARY_DIR}/usp10_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/usp10.def)

add_library(usp10 SHARED ${SOURCE})

set_module_type(usp10 win32dll)

target_link_libraries(usp10 wine)

add_importlibs(usp10 gdi32 msvcrt kernel32 ntdll)
add_importlib_target(usp10.spec)

add_cd_file(TARGET usp10 DESTINATION reactos/system32 FOR all)
