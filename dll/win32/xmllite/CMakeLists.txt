
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

if(MSVC)
    # Disable warning C4146: unary minus operator applied to unsigned type, result still unsigned
    target_compile_options(xmllite PRIVATE /wd4146)
endif()

set_module_type(xmllite win32dll)
target_link_libraries(xmllite uuid wine oldnames)
add_importlibs(xmllite msvcrt kernel32 ntdll)
add_pch(xmllite precomp.h SOURCE)
add_cd_file(TARGET xmllite DESTINATION reactos/system32 FOR all)
