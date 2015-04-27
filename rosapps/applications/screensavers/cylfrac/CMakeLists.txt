
add_executable(cylfrac cylfrac.c cylfrac.rc)
set_module_type(cylfrac win32gui UNICODE)
set_target_properties(cylfrac PROPERTIES SUFFIX ".scr")
target_link_libraries(cylfrac scrnsave)
add_importlibs(cylfrac user32 gdi32 opengl32 glu32 winmm msvcrt kernel32)
add_cd_file(TARGET cylfrac DESTINATION reactos/system32 FOR all)
