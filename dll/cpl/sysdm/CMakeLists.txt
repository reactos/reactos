
spec2def(sysdm.cpl sysdm.spec)

list(APPEND SOURCE
    advanced.c
    environment.c
    general.c
    hardprof.c
    hardware.c
    licence.c
    startrec.c
    sysdm.c
    userprofile.c
    virtmem.c
    precomp.h)

file(GLOB sysdm_rc_deps resources/*.*)
add_rc_deps(sysdm.rc ${sysdm_rc_deps})

add_library(sysdm SHARED
    ${SOURCE}
    sysdm.rc
    ${CMAKE_CURRENT_BINARY_DIR}/sysdm.def)

set_module_type(sysdm cpl UNICODE)
add_importlibs(sysdm advapi32 setupapi user32 gdi32 comctl32 msimg32 shell32 shlwapi ole32 powrprof msvcrt kernel32 ntdll)
add_pch(sysdm precomp.h SOURCE)
add_cd_file(TARGET sysdm DESTINATION reactos/system32 FOR all)
