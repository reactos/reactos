
set_unicode()
set_rc_compiler()

list(APPEND SOURCE
    about.c
    devmgmt.c
    enumdevices.c
    mainwnd.c
    misc.c
    devmgmt.rc)

add_executable(devmgmt ${SOURCE})

set_module_type(devmgmt win32gui)
add_importlibs(devmgmt setupapi gdi32 user32 comctl32 advapi32 devmgr msvcrt kernel32)
add_pch(devmgmt precomp.h)
add_cd_file(TARGET devmgmt DESTINATION reactos/system32 FOR all)
