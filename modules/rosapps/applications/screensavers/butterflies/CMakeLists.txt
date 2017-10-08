
add_executable(butterflies butterflies.c butterflies.rc)
set_module_type(butterflies win32gui UNICODE)
set_target_properties(butterflies PROPERTIES SUFFIX ".scr")
target_link_libraries(butterflies scrnsave)
add_importlibs(butterflies user32 gdi32 opengl32 glu32 advapi32 shell32 msvcrt kernel32)
add_cd_file(TARGET butterflies DESTINATION reactos/system32 FOR all)
