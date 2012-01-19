
set_rc_compiler()
spec2def(deskadp.dll deskadp.spec)

list(APPEND SOURCE
    deskadp.c
    shxiface.c
    deskadp.rc
    ${CMAKE_CURRENT_BINARY_DIR}/deskadp.def)

add_library(deskadp SHARED ${SOURCE})

set_module_type(deskadp win32dll UNICODE)

target_link_libraries(deskadp uuid)

add_importlibs(deskadp
    user32
    gdi32
    comctl32
    ole32
    msvcrt
    kernel32
    ntdll)

add_pch(deskadp precomp.h)

add_cd_file(TARGET deskadp DESTINATION reactos/system32 FOR all)
