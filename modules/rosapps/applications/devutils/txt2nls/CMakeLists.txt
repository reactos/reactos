
list(APPEND SOURCE
    main.c
    txt.c
    nls.c
    precomp.h)

add_executable(txt2nls ${SOURCE} txt2nls.rc)
add_pch(txt2nls precomp.h SOURCE)
set_module_type(txt2nls win32cui)
add_importlibs(txt2nls msvcrt kernel32)
add_cd_file(TARGET txt2nls DESTINATION reactos/system32 FOR all)
