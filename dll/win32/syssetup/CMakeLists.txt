
spec2def(syssetup.dll syssetup.spec)

list(APPEND SOURCE
    classinst.c
    dllmain.c
    install.c
    logfile.c
    netinstall.c
    proppage.c
    security.c
    wizard.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/syssetup_stubs.c)

add_library(syssetup MODULE
    ${SOURCE}
    syssetup.rc
    ${CMAKE_CURRENT_BINARY_DIR}/syssetup.def)

add_pch(syssetup precomp.h SOURCE)
set_module_type(syssetup win32dll UNICODE)
target_link_libraries(syssetup uuid wine ${PSEH_LIB})
add_importlibs(syssetup advapi32 gdi32 user32 samlib userenv comctl32 setupapi rpcrt4 ole32 shell32 shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET syssetup DESTINATION reactos/system32 FOR all)
