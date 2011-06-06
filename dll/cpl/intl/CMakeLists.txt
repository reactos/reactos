
set_unicode()
set_rc_compiler()

add_definitions(-D_DLL -D__USE_CRTIMP)

spec2def(intl.cpl intl.spec)

add_library(intl SHARED
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
    intl.rc)

set_module_type(intl cpl)

add_importlibs(intl
    msvcrt
    user32
    comctl32
    advapi32
    setupapi
    shell32
    kernel32)

add_cd_file(TARGET intl DESTINATION reactos/system32 FOR all)
