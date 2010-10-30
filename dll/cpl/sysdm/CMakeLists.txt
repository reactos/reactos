
set_unicode()

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
    sysdm.rc
    ${CMAKE_CURRENT_BINARY_DIR}/sysdm.def)

add_library(sysdm SHARED ${CMAKE_CURRENT_BINARY_DIR}/sysdm_precomp.h.gch ${SOURCE})

set_module_type(sysdm cpl)

add_importlibs(sysdm
    msvcrt
    advapi32
    setupapi
    user32
    gdi32
    comctl32
    msimg32
    shell32
    shlwapi
    ole32
    powrprof
    kernel32)

add_pch(sysdm ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_cab_target(sysdm 1)
