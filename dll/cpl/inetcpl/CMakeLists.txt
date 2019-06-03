
add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    connections.c
    content.c
    general.c
    inetcpl.c
    security.c
    precomp.h)

file(GLOB inetcpl_rc_deps resources/*.*)
add_rc_deps(inetcpl.rc ${inetcpl_rc_deps})

add_library(inetcpl MODULE
    ${SOURCE}
    inetcpl.rc)
spec2def(inetcpl inetcpl.cpl inetcpl.spec)

set_module_type(inetcpl cpl UNICODE)
target_link_libraries(inetcpl wine)
add_delay_importlibs(inetcpl cryptui wininet ole32 urlmon shell32)
add_importlibs(inetcpl advapi32 comctl32 user32 shlwapi msvcrt kernel32 ntdll)
add_pch(inetcpl precomp.h SOURCE)
add_cd_file(TARGET inetcpl DESTINATION reactos/system32 FOR all)
