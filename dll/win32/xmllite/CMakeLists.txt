
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(xmllite.dll xmllite.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    reader.c
    writer.c
    xmllite_main.c
    xmllite_private.h
    ${CMAKE_CURRENT_BINARY_DIR}/xmllite_stubs.c)

add_library(xmllite SHARED
    ${SOURCE}
    guid.c
    ${CMAKE_CURRENT_BINARY_DIR}/xmllite.def)

set_module_type(xmllite win32dll)
target_link_libraries(xmllite uuid wine)
add_importlibs(xmllite msvcrt kernel32 ntdll)
add_pch(xmllite xmllite_private.h SOURCE NO_UNITY_BUILD)
add_cd_file(TARGET xmllite DESTINATION reactos/system32 FOR all)
