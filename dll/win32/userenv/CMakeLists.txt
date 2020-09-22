
spec2def(userenv.dll userenv.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    desktop.c
    directory.c
    environment.c
    gpolicy.c
    misc.c
    profile.c
    registry.c
    setup.c
    sid.c
    userenv.c)

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/userenv_stubs.c)

add_library(userenv MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    userenv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/userenv.def)

set_module_type(userenv win32dll)
target_link_libraries(userenv uuid)
add_importlibs(userenv advapi32 user32 msvcrt kernel32 ntdll)
add_pch(userenv precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET userenv DESTINATION reactos/system32 FOR all)
