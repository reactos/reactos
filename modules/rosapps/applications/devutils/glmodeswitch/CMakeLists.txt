
add_executable(glmodeswitch glmodeswitch.c glmodeswitch.rc)
set_module_type(glmodeswitch win32gui)
add_importlibs(glmodeswitch opengl32 user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET glmodeswitch DESTINATION reactos/system32 FOR all)
