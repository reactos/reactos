
set_rc_compiler()
spec2def(fontext.dll fontext.spec)

add_library(fontext SHARED
    fontext.c
    regsvr.c
    fontext.rc
    ${CMAKE_CURRENT_BINARY_DIR}/fontext.def)

set_module_type(fontext win32dll UNICODE)

target_link_libraries(fontext uuid)

add_importlibs(fontext
    user32
    gdi32
    ole32
    shlwapi
    lz32
    advapi32
    setupapi
    msvcrt
    kernel32
    ntdll)

add_pch(fontext fontext.h)
add_cd_file(TARGET fontext DESTINATION reactos/system32 FOR all)
