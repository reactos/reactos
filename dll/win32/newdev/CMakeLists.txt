
set_unicode()

spec2def(newdev.dll newdev.spec)

list(APPEND SOURCE
    newdev.c
    stubs.c
    wizard.c
    newdev.rc
    ${CMAKE_CURRENT_BINARY_DIR}/newdev.def)

add_library(newdev SHARED ${SOURCE})

set_module_type(newdev win32dll)

target_link_libraries(newdev wine)

add_importlibs(newdev gdi32 comctl32 setupapi advapi32 user32 shell32 msvcrt kernel32 ntdll)

add_cab_target(newdev 1)
add_importlib_target(newdev.spec)
