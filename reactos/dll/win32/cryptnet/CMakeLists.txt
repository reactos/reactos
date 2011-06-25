
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(cryptnet.dll cryptnet.spec ADD_IMPORTLIB GENERATE_STUBS)

list(APPEND SOURCE
    cryptnet_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/cryptnet_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/cryptnet.def)

add_library(cryptnet SHARED ${SOURCE})

set_module_type(cryptnet win32dll)

target_link_libraries(cryptnet wine)

add_importlibs(cryptnet
    crypt32
    wininet
    msvcrt
    kernel32
    ntdll)

add_importlib_target(cryptnet.spec)
add_cd_file(TARGET cryptnet DESTINATION reactos/system32 FOR all)
