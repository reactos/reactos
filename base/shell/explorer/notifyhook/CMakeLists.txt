add_definitions(-D_NOTIFYHOOK_IMPL)

add_library(notifyhook SHARED notifyhook.c ${CMAKE_CURRENT_SOURCE_DIR}/notifyhook.rc)

set_module_type(notifyhook win32dll)

target_link_libraries(notifyhook 
    ${CMAKE_CURRENT_SOURCE_DIR}/notifyhook.def
    -lkernel32
    -luser32)

add_livecd_target(notifyhook reactos/system32)