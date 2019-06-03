
list(APPEND SOURCE
    deskadp.c
    shxiface.c
    precomp.h)

add_rc_deps(deskadp.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/adapter.ico)

add_library(deskadp MODULE
    ${SOURCE}
    deskadp.rc)
spec2def(deskadp deskadp.dll deskadp.spec)

set_module_type(deskadp win32dll UNICODE)
target_link_libraries(deskadp uuid)
add_importlibs(deskadp user32 gdi32 comctl32 ole32 msvcrt kernel32 ntdll)
add_pch(deskadp precomp.h SOURCE)
add_cd_file(TARGET deskadp DESTINATION reactos/system32 FOR all)
