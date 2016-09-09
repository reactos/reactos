
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(sxs.dll sxs.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cache.c
    name.c
    sxs.c
    sxs_private.h
    ${CMAKE_CURRENT_BINARY_DIR}/sxs_stubs.c)

add_library(sxs SHARED
    ${SOURCE}
    guid.c
    ${CMAKE_CURRENT_BINARY_DIR}/sxs.def)

set_module_type(sxs win32dll)
target_link_libraries(sxs wine)
add_importlibs(sxs oleaut32 ole32 msvcrt kernel32 ntdll)
add_pch(sxs sxs_private.h SOURCE)
add_cd_file(TARGET sxs DESTINATION reactos/system32 FOR all)
