
list(APPEND SOURCE
    misc.c
    logoff.c
    precomp.h)

add_executable(logoff ${SOURCE} logoff.rc)
set_module_type(logoff win32cui)
add_importlibs(logoff advapi32 user32 msvcrt kernel32)
add_pch(logoff precomp.h SOURCE)
add_cd_file(TARGET logoff DESTINATION reactos/system32 FOR all)
