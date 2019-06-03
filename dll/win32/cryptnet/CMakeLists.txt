
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    cryptnet_main.c)

add_library(cryptnet MODULE ${SOURCE})
spec2def(cryptnet cryptnet.dll cryptnet.spec ADD_IMPORTLIB)

set_module_type(cryptnet win32dll)
target_link_libraries(cryptnet wine)
add_delay_importlibs(cryptnet wininet)
add_importlibs(cryptnet crypt32 msvcrt kernel32 ntdll)
add_cd_file(TARGET cryptnet DESTINATION reactos/system32 FOR all)
