
spec2def(fmifs.dll fmifs.spec)

list(APPEND SOURCE
    chkdsk.c
    compress.c
    diskcopy.c
    extend.c
    format.c
    init.c
    media.c
    query.c
    fmifs.rc
    ${CMAKE_CURRENT_BINARY_DIR}/fmifs_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/fmifs.def)

add_library(fmifs SHARED ${SOURCE})

add_pch(fmifs precomp.h)

set_entrypoint(fmifs InitializeFmIfs@12)

add_importlibs(fmifs kernel32 ntdll)
add_dependencies(fmifs psdk)
add_cd_file(TARGET fmifs DESTINATION reactos/system32 FOR all)
add_importlib_target(fmifs.spec)
