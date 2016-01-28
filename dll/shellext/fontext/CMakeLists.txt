
spec2def(fontext.dll fontext.spec)

list(APPEND SOURCE
    fontext.c
    regsvr.c
    fontext.h)

add_library(fontext SHARED
    ${SOURCE}
    fontext.rc
    ${CMAKE_CURRENT_BINARY_DIR}/fontext.def)

set_module_type(fontext win32dll UNICODE)
target_link_libraries(fontext uuid)
add_importlibs(fontext user32 gdi32 ole32 shlwapi lz32 advapi32 setupapi msvcrt kernel32 ntdll)
add_pch(fontext fontext.h SOURCE)
add_cd_file(TARGET fontext DESTINATION reactos/system32 FOR all)
