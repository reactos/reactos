
set_unicode()

add_definitions(-D_DLL -D__USE_CRTIMP)

spec2def(syssetup ${CMAKE_CURRENT_SOURCE_DIR}/syssetup.spec ${CMAKE_CURRENT_BINARY_DIR}/syssetup.def)

add_library(syssetup SHARED
    classinst.c
    dllmain.c
    install.c
    logfile.c
    wizard.c
    syssetup.rc)

set_module_type(syssetup win32dll)

target_link_libraries(syssetup
    ${CMAKE_CURRENT_BINARY_DIR}/syssetup.def
    uuid
    pseh)

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

add_dependencies(syssetup syssetup_def)
add_livecd_target(syssetup reactos/system32)
