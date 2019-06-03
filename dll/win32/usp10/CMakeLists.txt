
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    bidi.c
    bracket.c
    breaking.c
    indic.c
    indicsyllable.c
    linebreak.c
    mirror.c
    opentype.c
    shape.c
    shaping.c
    usp10.c
    precomp.h)

add_library(usp10 MODULE
    ${SOURCE})
spec2def(usp10 usp10.dll usp10.spec ADD_IMPORTLIB)

set_module_type(usp10 win32dll)
target_link_libraries(usp10 wine)
add_importlibs(usp10 advapi32 user32 gdi32 msvcrt kernel32 ntdll)
add_pch(usp10 precomp.h SOURCE)
add_cd_file(TARGET usp10 DESTINATION reactos/system32 FOR all)
