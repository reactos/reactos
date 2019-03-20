
spec2def(wlnotify.dll wlnotify.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    scard.c
    schedsvc.c
    senssvc.c
    termserv.c
    wlballoon.c
    wlnotify.c
    precomp.h)

add_library(wlnotify MODULE
    ${SOURCE}
    wlnotify.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wlnotify.def)

set_module_type(wlnotify win32dll UNICODE ENTRYPOINT DllMain 12)
target_link_libraries(wlnotify wine)
add_importlibs(wlnotify advapi32 kernel32 ntdll)
add_pch(wlnotify precomp.h SOURCE)
add_cd_file(TARGET wlnotify DESTINATION reactos/system32 FOR all)
