
set_unicode()
set_rc_compiler()

list(APPEND SOURCE
    toolspage.c
    srvpage.c
    systempage.c
    startuppage.c
    freeldrpage.c
    generalpage.c
    msconfig.c
    msconfig.rc)

add_executable(msconfig ${CMAKE_CURRENT_BINARY_DIR}/msconfig_precomp.h.gch ${SOURCE})

add_pch(msconfig ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

set_module_type(msconfig win32gui)
add_importlibs(msconfig user32 advapi32 version comctl32 shell32 shlwapi msvcrt kernel32)
add_cd_file(TARGET msconfig DESTINATION reactos/system32 FOR all)
