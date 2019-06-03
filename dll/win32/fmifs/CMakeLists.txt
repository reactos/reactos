
list(APPEND SOURCE
    chkdsk.c
    compress.c
    diskcopy.c
    extend.c
    format.c
    init.c
    media.c
    query.c
    precomp.h)

add_library(fmifs MODULE
    ${SOURCE}
    fmifs.rc)
spec2def(fmifs fmifs.dll fmifs.spec ADD_IMPORTLIB)

set_module_type(fmifs win32dll UNICODE ENTRYPOINT InitializeFmIfs 12)
add_importlibs(fmifs kernel32 ntdll)
add_pch(fmifs precomp.h SOURCE)
add_dependencies(fmifs psdk)
add_cd_file(TARGET fmifs DESTINATION reactos/system32 FOR all)
