
set_unicode()

spec2def(newdev ${CMAKE_CURRENT_SOURCE_DIR}/newdev.spec ${CMAKE_CURRENT_BINARY_DIR}/newdev.def)

add_library(newdev SHARED
    newdev.c
    stubs.c
    wizard.c
    newdev.rc)

set_module_type(newdev win32dll)

target_link_libraries(newdev
    ${CMAKE_CURRENT_BINARY_DIR}/newdev.def
    wine)

add_importlibs(newdev gdi32 comctl32 setupapi advapi32 user32 shell32 kernel32 ntdll)
add_dependencies(newdev newdev_def)
