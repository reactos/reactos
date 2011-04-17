
set_unicode()
set_rc_compiler()

list(APPEND SOURCE cacls.c cacls.rc)

add_executable(cacls
    ${CMAKE_CURRENT_BINARY_DIR}/cacls_precomp.h.gch
    ${SOURCE})

add_pch(cacls ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

set_module_type(cacls win32cui)
add_importlibs(cacls advapi32 user32 shell32 msvcrt kernel32)
add_cab_target(cacls 1)
