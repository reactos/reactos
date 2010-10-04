
list(APPEND SOURCE
    environment.c
    sas.c
    screensaver.c
    setup.c
    winlogon.c
    wlx.c
    winlogon.rc)

add_executable(winlogon ${CMAKE_CURRENT_BINARY_DIR}/winlogon_winlogon.h.gch ${SOURCE})

add_pch(winlogon ${CMAKE_CURRENT_SOURCE_DIR}/winlogon.h ${SOURCE})

set_module_type(winlogon win32gui)

add_importlibs(winlogon user32 advapi32 userenv secur32 msvcrt)
target_link_libraries(winlogon wine)
