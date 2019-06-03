
list(APPEND SOURCE
    vdmdbg.c)

add_library(vdmdbg MODULE ${SOURCE})
spec2def(vdmdbg vdmdbg.dll vdmdbg.spec)

set_module_type(vdmdbg win32dll UNICODE)
add_importlibs(vdmdbg msvcrt kernel32 ntdll)
add_cd_file(TARGET vdmdbg DESTINATION reactos/system32 FOR all)
