
spec2def(deskmon.dll deskmon.spec)

list(APPEND SOURCE
    deskmon.c
    shxiface.c
    deskmon.rc
    ${CMAKE_CURRENT_BINARY_DIR}/deskmon.def)

add_library(deskmon SHARED ${SOURCE})
set_module_type(deskmon win32dll UNICODE)
target_link_libraries(deskmon uuid)

add_importlibs(deskmon
    user32
    gdi32
    comctl32
    ole32
    msvcrt
    kernel32
    ntdll)

add_pch(deskmon precomp.h)
add_cd_file(TARGET deskmon DESTINATION reactos/system32 FOR all)
