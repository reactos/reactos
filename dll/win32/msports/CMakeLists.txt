
spec2def(msports.dll msports.spec)

list(APPEND SOURCE
    classinst.c
    comdb.c
    msports.c
    parallel.c
    serial.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/msports_stubs.c)

add_library(msports MODULE
    ${SOURCE}
    msports.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msports.def)

set_module_type(msports win32dll UNICODE)
target_link_libraries(msports wine)
add_importlibs(msports setupapi comctl32 user32 advapi32 msvcrt kernel32 ntdll)
add_pch(msports precomp.h SOURCE)
add_cd_file(TARGET msports DESTINATION reactos/system32 FOR all)
