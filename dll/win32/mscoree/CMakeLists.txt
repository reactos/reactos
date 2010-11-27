
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mscoree.dll mscoree.spec)

add_library(mscoree SHARED corruntimehost.c mscoree_main.c ${CMAKE_CURRENT_BINARY_DIR}/mscoree.def)

set_module_type(mscoree win32dll)

target_link_libraries(mscoree
    wine
    uuid)

add_importlibs(mscoree advapi32 shell32 msvcrt kernel32 ntdll)

add_cab_target(mscoree 1)
