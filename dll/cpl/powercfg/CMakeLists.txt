
spec2def(powercfg.cpl powercfg.spec)

list(APPEND SOURCE
    powercfg.c
    powershemes.c
    alarms.c
    advanced.c
    hibernate.c
    powermeter.c
    powercfg.h)

file(GLOB powercfg_rc_deps resources/*.*)
add_rc_deps(powercfg.rc ${powercfg_rc_deps})

add_library(powercfg MODULE
    ${SOURCE}
    powercfg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/powercfg.def)

set_module_type(powercfg cpl UNICODE)
add_importlibs(powercfg shlwapi user32 powrprof comctl32 shell32 advapi32 msvcrt setupapi kernel32 ntdll)
add_pch(powercfg powercfg.h SOURCE)
add_cd_file(TARGET powercfg DESTINATION reactos/system32 FOR all)
