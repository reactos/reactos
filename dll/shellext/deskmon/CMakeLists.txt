
list(APPEND SOURCE
    deskmon.c
    shxiface.c
    precomp.h)

add_rc_deps(deskmon.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/monitor.ico)

add_library(deskmon MODULE
    ${SOURCE}
    deskmon.rc)
spec2def(deskmon deskmon.dll deskmon.spec)

set_module_type(deskmon win32dll UNICODE)
target_link_libraries(deskmon uuid)
add_importlibs(deskmon user32 gdi32 comctl32 ole32 msvcrt kernel32 ntdll)
add_pch(deskmon precomp.h SOURCE)
add_cd_file(TARGET deskmon DESTINATION reactos/system32 FOR all)
