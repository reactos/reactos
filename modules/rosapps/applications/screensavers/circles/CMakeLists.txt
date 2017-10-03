
add_executable(circles circles.c circles.rc)
set_module_type(circles win32gui UNICODE)
set_target_properties(circles PROPERTIES SUFFIX ".scr")
target_link_libraries(circles scrnsave)
add_importlibs(circles user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET circles DESTINATION reactos/system32 FOR all)
