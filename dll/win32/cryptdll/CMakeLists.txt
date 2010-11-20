
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(cryptdll.dll cryptdll.spec)

add_library(cryptdll SHARED
            cryptdll.c
            stubs.c
            ${CMAKE_CURRENT_BINARY_DIR}/cryptdll.def)

set_module_type(cryptdll win32dll)

target_link_libraries(cryptdll
    ${CMAKE_CURRENT_BINARY_DIR}/cryptdll.def
    wine)

add_importlibs(cryptdll kernel32 ntdll)

add_cab_target(cryptdll 1)
