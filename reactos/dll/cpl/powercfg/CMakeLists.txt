
set_rc_compiler()

spec2def(powercfg.cpl powercfg.spec)

add_library(powercfg SHARED
    powercfg.c
    powershemes.c
    alarms.c
    advanced.c
    hibernate.c
    powercfg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/powercfg.def)

set_module_type(powercfg cpl UNICODE)

add_importlibs(powercfg
    msvcrt
    user32
    powrprof
    comctl32
    shell32
    advapi32
    kernel32
    ntdll)

add_pch(powercfg powercfg.h)
add_cd_file(TARGET powercfg DESTINATION reactos/system32 FOR all)
