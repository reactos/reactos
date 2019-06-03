
list(APPEND SOURCE
    classinst.c
    comdb.c
    msports.c
    parallel.c
    serial.c
    precomp.h)

add_library(msports MODULE
    ${SOURCE}
    msports.rc)
spec2def(msports msports.dll msports.spec)

set_module_type(msports win32dll UNICODE)
target_link_libraries(msports wine)
add_importlibs(msports setupapi comctl32 user32 advapi32 msvcrt kernel32 ntdll)
add_pch(msports precomp.h SOURCE)
add_cd_file(TARGET msports DESTINATION reactos/system32 FOR all)
