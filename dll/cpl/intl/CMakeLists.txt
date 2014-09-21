
spec2def(intl.cpl intl.spec)

list(APPEND SOURCE
    currency.c
    date.c
    generalp.c
    intl.c
    inplocale.c
    numbers.c
    time.c
    misc.c
    kblayouts.c
    languages.c
    advanced.c
    sort.c
    intl.h)

add_library(intl SHARED
    ${SOURCE}
    intl.rc
    ${CMAKE_CURRENT_BINARY_DIR}/intl.def)

set_module_type(intl cpl UNICODE)

add_importlibs(intl
    msvcrt
    user32
    comctl32
    advapi32
    setupapi
    shell32
    kernel32
    ntdll)

add_pch(intl intl.h SOURCE)
add_cd_file(TARGET intl DESTINATION reactos/system32 FOR all)
