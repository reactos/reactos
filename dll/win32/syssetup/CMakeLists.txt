
set_unicode()

add_definitions(-D_DLL -D__USE_CRTIMP)

spec2def(syssetup.dll syssetup.spec)

add_library(syssetup SHARED
    classinst.c
    dllmain.c
    install.c
    logfile.c
    wizard.c
    syssetup.rc
    ${CMAKE_CURRENT_BINARY_DIR}/syssetup.def)

set_module_type(syssetup win32dll)

target_link_libraries(syssetup
    uuid
    ${PSEH_LIB})

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


add_cab_target(syssetup 1)
