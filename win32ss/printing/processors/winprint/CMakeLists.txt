
list(APPEND SOURCE
    main.c
    raw.c
    precomp.h)

add_library(winprint MODULE
    ${SOURCE}
    winprint.rc)
spec2def(winprint winprint.dll winprint.spec ADD_IMPORTLIB)

set_module_type(winprint win32dll UNICODE)
target_link_libraries(winprint wine)
add_importlibs(winprint spoolss msvcrt kernel32 ntdll)
add_pch(winprint precomp.h SOURCE)
add_cd_file(TARGET winprint DESTINATION reactos/system32/spool/prtprocs/w32x86 FOR all)
