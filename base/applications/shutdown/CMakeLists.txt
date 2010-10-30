
list(APPEND SOURCE
    misc.c
    shutdown.c
    shutdown.rc)

add_executable(shutdown ${CMAKE_CURRENT_BINARY_DIR}/shutdown_precomp.h.gch ${SOURCE})

add_pch(shutdown ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

set_module_type(shutdown win32cui)

add_importlibs(shutdown advapi32 user32 msvcrt kernel32)
add_cab_target(shutdown 1)