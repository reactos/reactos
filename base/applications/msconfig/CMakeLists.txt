
list(APPEND SOURCE
    toolspage.c
    srvpage.c
    systempage.c
    startuppage.c
    freeldrpage.c
    generalpage.c
    msconfig.c
    precomp.h)

add_rc_deps(msconfig.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/msconfig.ico)
add_executable(msconfig ${SOURCE} msconfig.rc)
set_module_type(msconfig win32gui UNICODE)
add_importlibs(msconfig user32 advapi32 version comctl32 shell32 shlwapi msvcrt kernel32)
add_pch(msconfig precomp.h SOURCE)
add_cd_file(TARGET msconfig DESTINATION reactos/system32 FOR all)
