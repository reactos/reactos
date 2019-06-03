
list(APPEND SOURCE
    newdev.c
    stubs.c
    wizard.c
    newdev_private.h)

add_library(newdev MODULE
    ${SOURCE}
    newdev.rc)
spec2def(newdev newdev.dll newdev.spec ADD_IMPORTLIB)

set_module_type(newdev win32dll UNICODE)
target_link_libraries(newdev wine)
add_importlibs(newdev gdi32 comctl32 setupapi advapi32 user32 shell32 msvcrt kernel32 ntdll)
add_pch(newdev newdev_private.h SOURCE)
add_cd_file(TARGET newdev DESTINATION reactos/system32 FOR all)
