
add_definitions(-D_NOTIFYHOOK_IMPL)

list(APPEND SOURCE
    notifyhook.c
    notifyhook.rc)

add_library(notifyhook MODULE ${SOURCE})
spec2def(notifyhook notifyhook.dll notifyhook.spec ADD_IMPORTLIB)

set_module_type(notifyhook win32dll)
add_importlibs(notifyhook user32 msvcrt kernel32)
add_cd_file(TARGET notifyhook DESTINATION reactos/system32 FOR all)
