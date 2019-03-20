
spec2def(liccpa.cpl liccpa.spec)

list(APPEND SOURCE
    liccpa.c
    liccpa.h)

file(GLOB liccpa_rc_deps resources/*.*)
add_rc_deps(liccpa.rc ${liccpa_rc_deps})

add_library(liccpa MODULE
    ${SOURCE}
    liccpa.rc
    ${CMAKE_CURRENT_BINARY_DIR}/liccpa.def)

set_module_type(liccpa cpl UNICODE)
add_importlibs(liccpa advapi32 user32 gdi32 msvcrt kernel32 ntdll)
add_pch(liccpa liccpa.h SOURCE)
add_cd_file(TARGET liccpa DESTINATION reactos/system32 FOR all)
