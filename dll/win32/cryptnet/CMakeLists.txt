
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(cryptnet.dll cryptnet.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cryptnet_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/cryptnet_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/cryptnet.def)

add_library(cryptnet MODULE ${SOURCE})
set_module_type(cryptnet win32dll)
target_link_libraries(cryptnet wine)
add_delay_importlibs(cryptnet wininet)
add_importlibs(cryptnet crypt32 msvcrt kernel32 ntdll)
add_cd_file(TARGET cryptnet DESTINATION reactos/system32 FOR all)
