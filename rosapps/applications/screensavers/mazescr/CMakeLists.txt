
add_executable(mazescr maze.c maze.rc)

set_module_type(mazescr win32gui UNICODE)
set_target_properties(mazescr PROPERTIES SUFFIX ".scr")
target_link_libraries(mazescr scrnsave)
add_importlibs(mazescr user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET mazescr DESTINATION reactos/system32 FOR all)
