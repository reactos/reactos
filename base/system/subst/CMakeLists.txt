
add_executable(subst subst.c subst.rc)
set_module_type(subst win32cui UNICODE)
add_importlibs(subst msvcrt kernel32 user32 ntdll)
add_cd_file(TARGET subst DESTINATION reactos/system32 FOR all)
