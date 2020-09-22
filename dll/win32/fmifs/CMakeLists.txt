
spec2def(fmifs.dll fmifs.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    chkdsk.c
    compress.c
    diskcopy.c
    extend.c
    format.c
    init.c
    media.c
    query.c)

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/fmifs_stubs.c)

add_library(fmifs MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    fmifs.rc
    ${CMAKE_CURRENT_BINARY_DIR}/fmifs.def)

set_module_type(fmifs win32dll UNICODE ENTRYPOINT InitializeFmIfs 12)
add_importlibs(fmifs kernel32 ntdll)
add_pch(fmifs precomp.h "${PCH_SKIP_SOURCE}")
add_dependencies(fmifs psdk)
add_cd_file(TARGET fmifs DESTINATION reactos/system32 FOR all)
