list(APPEND SOURCE
    common.c
    dirty.c
    fsinfo.c
    fsutil.c
    hardlink.c
    volume.c
    fsutil.h)
add_executable(fsutil ${SOURCE} fsutil.rc)
set_module_type(fsutil win32cui UNICODE)
add_importlibs(fsutil msvcrt kernel32 ntdll)
add_pch(fsutil fsutil.h SOURCE)
add_cd_file(TARGET fsutil DESTINATION reactos/system32 FOR all)
