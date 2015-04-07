
add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(advpack.dll advpack.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    advpack.c
    files.c
    install.c
    reg.c
    advpack_private.h)

add_library(advpack SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/advpack.def)

set_module_type(advpack win32dll)
target_link_libraries(advpack wine)
add_importlibs(advpack ole32 setupapi version advapi32 msvcrt kernel32 ntdll)
add_pch(advpack advpack_private.h SOURCE)
add_cd_file(TARGET advpack DESTINATION reactos/system32 FOR all)
