
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(xmllite.dll xmllite.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    reader.c
    writer.c
    xmllite_main.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/xmllite_stubs.c)

add_library(xmllite MODULE
    ${SOURCE}
    guid.c
    ${CMAKE_CURRENT_BINARY_DIR}/xmllite.def)

set_module_type(xmllite win32dll)
target_link_libraries(xmllite uuid wine)
add_importlibs(xmllite msvcrt kernel32 ntdll)
add_pch(xmllite precomp.h SOURCE)
add_cd_file(TARGET xmllite DESTINATION reactos/system32 FOR all)
