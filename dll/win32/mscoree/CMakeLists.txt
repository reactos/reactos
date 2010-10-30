
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mscoree ${CMAKE_CURRENT_SOURCE_DIR}/mscoree.spec ${CMAKE_CURRENT_BINARY_DIR}/mscoree.def)

add_library(mscoree SHARED corruntimehost.c mscoree_main.c)

set_module_type(mscoree win32dll)

target_link_libraries(mscoree
    ${CMAKE_CURRENT_BINARY_DIR}/mscoree.def
    wine
    uuid)

add_importlibs(mscoree advapi32 shell32 kernel32 ntdll)
add_dependencies(mscoree mscoree_def)
add_cab_target(mscoree 1)
