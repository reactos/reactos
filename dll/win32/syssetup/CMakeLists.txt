
list(APPEND SOURCE
    classinst.c
    dllmain.c
    install.c
    logfile.c
    proppage.c
    security.c
    wizard.c
    precomp.h)

add_library(syssetup MODULE
    ${SOURCE}
    syssetup.rc)
spec2def(syssetup syssetup.dll syssetup.spec)

add_pch(syssetup precomp.h SOURCE)
set_module_type(syssetup win32dll UNICODE)
target_link_libraries(syssetup uuid wine ${PSEH_LIB})
add_importlibs(syssetup advapi32 gdi32 user32 samlib userenv comctl32 setupapi ole32 shell32 shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET syssetup DESTINATION reactos/system32 FOR all)
