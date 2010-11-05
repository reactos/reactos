add_definitions(-D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    misc.c
    logoff.c
    logoff.rc)

add_executable(logoff ${CMAKE_CURRENT_BINARY_DIR}/logoff_precomp.h.gch ${SOURCE})

add_pch(logoff ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

set_module_type(logoff win32cui)

add_importlibs(logoff advapi32 user32 msvcrt kernel32)
