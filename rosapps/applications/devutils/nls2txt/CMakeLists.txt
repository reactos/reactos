
list(APPEND SOURCE
    main.c
    nls.c
    bestfit.c
    precomp.h)

add_executable(nls2txt ${SOURCE} nls2txt.rc)
add_pch(nls2txt precomp.h SOURCE)
set_module_type(nls2txt win32cui UNICODE)
add_importlibs(nls2txt getuname msvcrt kernel32)
add_cd_file(TARGET nls2txt DESTINATION reactos/system32 FOR all)
