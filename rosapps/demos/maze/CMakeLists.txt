
add_executable(maze maze.c maze.rc)
set_module_type(maze win32gui)
add_importlibs(maze user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET maze DESTINATION reactos/system32 FOR all)
