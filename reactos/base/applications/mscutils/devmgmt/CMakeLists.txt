
list(APPEND SOURCE
    about.c
    devmgmt.c
    enumdevices.c
    mainwnd.c
    misc.c
    precomp.h)

file(GLOB devmgmt_rc_deps res/*.*)
add_rc_deps(devmgmt.rc ${devmgmt_rc_deps})
add_executable(devmgmt ${SOURCE} devmgmt.rc)
set_module_type(devmgmt win32gui UNICODE)
add_importlibs(devmgmt setupapi gdi32 user32 comctl32 advapi32 devmgr msvcrt kernel32)
add_pch(devmgmt precomp.h SOURCE)
add_cd_file(TARGET devmgmt DESTINATION reactos/system32 FOR all)
