
set_unicode()

list(APPEND SOURCE
    about.c
    devmgmt.c
    enumdevices.c
    mainwnd.c
    misc.c
    devmgmt.rc)

add_executable(devmgmt ${CMAKE_CURRENT_BINARY_DIR}/devmgmt_precomp.h.gch ${SOURCE})

add_pch(devmgmt ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

set_module_type(devmgmt win32gui)
add_importlibs(devmgmt setupapi gdi32 user32 comctl32 advapi32 devmgr msvcrt kernel32)
add_cd_file(TARGET devmgmt DESTINATION reactos/system32)
