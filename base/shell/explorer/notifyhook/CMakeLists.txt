
add_definitions(-D_NOTIFYHOOK_IMPL)

add_library(notifyhook SHARED
    notifyhook.c
    notifyhook.rc
    notifyhook.def)

set_module_type(notifyhook win32dll)

add_importlibs(notifyhook user32 msvcrt kernel32)

add_cab_target(notifyhook 1)
