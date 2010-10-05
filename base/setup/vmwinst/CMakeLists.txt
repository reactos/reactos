
set_unicode()

add_executable(vmwinst
    vmwinst.c
    vmwinst.rc)

target_link_libraries(vmwinst pseh)

set_module_type(vmwinst win32gui)
add_importlibs(vmwinst
    advapi32
    comctl32
    comdlg32
    newdev
    user32
    setupapi
    shell32
    msvcrt)
