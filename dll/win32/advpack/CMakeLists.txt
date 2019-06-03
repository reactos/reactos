
add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    advpack.c
    files.c
    install.c
    reg.c
    precomp.h)

add_library(advpack MODULE
    ${SOURCE}
    advpack.rc)
spec2def(advpack advpack.dll advpack.spec ADD_IMPORTLIB)

set_module_type(advpack win32dll)
target_link_libraries(advpack wine)
add_importlibs(advpack ole32 setupapi version advapi32 msvcrt kernel32 ntdll)
add_pch(advpack precomp.h SOURCE)
add_cd_file(TARGET advpack DESTINATION reactos/system32 FOR all)
