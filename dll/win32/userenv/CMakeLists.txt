
spec2def(userenv ${CMAKE_CURRENT_SOURCE_DIR}/userenv.spec ${CMAKE_CURRENT_BINARY_DIR}/userenv.def)

list(APPEND SOURCE
    desktop.c
    directory.c
    environment.c
    gpolicy.c
    misc.c
    profile.c
    registry.c
    setup.c
    userenv.c
    userenv.rc)

add_library(userenv SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/userenv_precomp.h.gch
    ${SOURCE})

set_module_type(userenv win32dll)

target_link_libraries(userenv
    ${CMAKE_CURRENT_BINARY_DIR}/userenv.def
    uuid)

add_importlibs(userenv advapi32 user32 kernel32 ntdll)
add_pch(userenv ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(userenv userenv_def)
add_livecd_target(userenv reactos/system32)
