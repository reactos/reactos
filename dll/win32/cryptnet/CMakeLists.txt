
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(cryptnet ${CMAKE_CURRENT_SOURCE_DIR}/cryptnet.spec ${CMAKE_CURRENT_BINARY_DIR}/cryptnet.def)

add_library(cryptnet SHARED cryptnet_main.c)

set_module_type(cryptnet win32dll)

target_link_libraries(cryptnet
    ${CMAKE_CURRENT_BINARY_DIR}/cryptnet.def
    wine)

add_importlibs(cryptnet
    crypt32
    wininet
    kernel32
    ntdll)

add_dependencies(cryptnet cryptnet_def)
add_cab_target(cryptnet 1)
