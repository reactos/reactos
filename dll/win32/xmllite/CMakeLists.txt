
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(xmllite.dll xmllite.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    reader.c
    writer.c
    xmllite_main.c
    guid.c
    ${CMAKE_CURRENT_BINARY_DIR}/xmllite_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/xmllite.def)

add_library(xmllite SHARED ${SOURCE})
set_module_type(xmllite win32dll)
target_link_libraries(xmllite uuid wine)
add_importlibs(xmllite msvcrt kernel32 ntdll)
add_cd_file(TARGET xmllite DESTINATION reactos/system32 FOR all)
