
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(sxs.dll sxs.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cache.c
    name.c
    sxs.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(sxs MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/sxs_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/sxs.def)

set_module_type(sxs win32dll)
target_link_libraries(sxs wine)
add_importlibs(sxs oleaut32 ole32 msvcrt kernel32 ntdll)
add_pch(sxs precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET sxs DESTINATION reactos/system32 FOR all)
