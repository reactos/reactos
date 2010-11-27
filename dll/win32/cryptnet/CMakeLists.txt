
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(cryptnet.dll cryptnet.spec)

add_library(cryptnet SHARED cryptnet_main.c ${CMAKE_CURRENT_BINARY_DIR}/cryptnet.def)

set_module_type(cryptnet win32dll)

target_link_libraries(cryptnet wine)

add_importlibs(cryptnet
    crypt32
    wininet
    msvcrt
    kernel32
    ntdll)


add_cab_target(cryptnet 1)
