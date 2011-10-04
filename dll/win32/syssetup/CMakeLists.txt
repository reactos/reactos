
set_rc_compiler()

spec2def(syssetup.dll syssetup.spec)

list(APPEND SOURCE
    classinst.c
    dllmain.c
    install.c
    logfile.c
    wizard.c
    syssetup.rc
    ${CMAKE_CURRENT_BINARY_DIR}/syssetup_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/syssetup.def)

add_library(syssetup SHARED ${SOURCE})
add_pch(syssetup precomp.h)
set_module_type(syssetup win32dll UNICODE)
target_link_libraries(syssetup uuid ${PSEH_LIB})

add_importlibs(syssetup
    msvcrt
    advapi32
    gdi32
    user32
    samlib
    userenv
    comctl32
    setupapi
    ole32
    shell32
    shlwapi
    kernel32
    ntdll)

add_cd_file(TARGET syssetup DESTINATION reactos/system32 FOR all)
