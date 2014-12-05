
add_definitions(-D_NOTIFYHOOK_IMPL)

spec2def(notifyhook.dll notifyhook.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    notifyhook.c
    notifyhook.rc
    ${CMAKE_CURRENT_BINARY_DIR}/notifyhook.def)

add_library(notifyhook SHARED ${SOURCE})

set_module_type(notifyhook win32dll)

add_importlibs(notifyhook user32 msvcrt kernel32)

add_cd_file(TARGET notifyhook DESTINATION reactos/system32 FOR all)
