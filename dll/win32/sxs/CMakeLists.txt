
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    cache.c
    name.c
    sxs.c
    precomp.h)

add_library(sxs MODULE
    ${SOURCE}
    guid.c)
spec2def(sxs sxs.dll sxs.spec ADD_IMPORTLIB)

set_module_type(sxs win32dll)
target_link_libraries(sxs wine)
add_importlibs(sxs oleaut32 ole32 msvcrt kernel32 ntdll)
add_pch(sxs precomp.h SOURCE)
add_cd_file(TARGET sxs DESTINATION reactos/system32 FOR all)
