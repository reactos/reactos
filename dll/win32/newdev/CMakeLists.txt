
set_unicode()

spec2def(newdev.dll newdev.spec)

add_library(newdev SHARED
    newdev.c
    stubs.c
    wizard.c
    newdev.rc
    ${CMAKE_CURRENT_BINARY_DIR}/newdev.def)

set_module_type(newdev win32dll)

target_link_libraries(newdev wine)

add_importlibs(newdev gdi32 comctl32 setupapi advapi32 user32 shell32 kernel32 ntdll)

add_cab_target(newdev 1)
