
set_unicode()

spec2def(sysdm ${CMAKE_CURRENT_SOURCE_DIR}/sysdm.spec ${CMAKE_CURRENT_BINARY_DIR}/sysdm.def)

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
    sysdm.rc)

add_library(sysdm SHARED ${CMAKE_CURRENT_BINARY_DIR}/sysdm_precomp.h.gch ${SOURCE})

set_module_type(sysdm cpl)

target_link_libraries(sysdm ${CMAKE_CURRENT_BINARY_DIR}/sysdm.def)

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
add_dependencies(sysdm sysdm_def)
add_livecd_target(sysdm reactos/system32)
