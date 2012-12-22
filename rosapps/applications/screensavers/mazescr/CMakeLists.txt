
add_executable(mazescr
    scrnsave.c
    maze.c
    scrnsave.rc)

set_module_type(mazescr win32gui)
set_target_properties(mazescr PROPERTIES SUFFIX ".scr")
add_importlibs(mazescr user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET mazescr DESTINATION reactos/system32 FOR all)
