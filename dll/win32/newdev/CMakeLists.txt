

spec2def(newdev.dll newdev.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    newdev.c
    stubs.c
    wizard.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/newdev_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/newdev.def)

add_library(newdev SHARED ${SOURCE})
set_module_type(newdev win32dll UNICODE)
target_link_libraries(newdev wine)
add_importlibs(newdev gdi32 comctl32 setupapi advapi32 user32 shell32 msvcrt kernel32 ntdll)
add_pch(newdev newdev_private.h)
add_cd_file(TARGET newdev DESTINATION reactos/system32 FOR all)

